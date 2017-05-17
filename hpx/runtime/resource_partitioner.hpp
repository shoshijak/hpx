//  Copyright (c)      2017 Shoshana Jakobovits
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_RESOURCE_PARTITIONER)
#define HPX_RESOURCE_PARTITIONER

#include <hpx/runtime/threads/coroutines/detail/coroutine_self.hpp>
#include <hpx/runtime/threads/policies/hwloc_topology_info.hpp>
#include <hpx/runtime/threads/policies/topology.hpp>
#include <hpx/runtime/threads/threadmanager.hpp>
#include <hpx/runtime/threads/topology.hpp>
#include <hpx/util/command_line_handling.hpp>
#include <hpx/util/thread_specific_ptr.hpp>

#include <boost/atomic.hpp>

#include <vector>
#include <string>
#include <stdexcept>
#include <algorithm>

namespace hpx {

    namespace resource {


    // scheduler assigned to thread_pool
    // choose same names as in command-line options except with _ instead of -
    enum scheduling_policy {
        unspecified = -1,
        local = 0,
        local_priority_fifo = 1,
        local_priority_lifo = 2,
        static_ = 3,
        static_priority = 4,
        abp_priority = 5,
        hierarchy = 6,
        periodic_priority = 7,
        throttle = 8
    };

    // structure used to encapsulate all characteristics of thread_pools
    // as specified by the user in int main()
    class init_pool_data {
    public:

        init_pool_data(std::string name, scheduling_policy = scheduling_policy::unspecified);

        // set functions
        void set_scheduler(scheduling_policy sched);

        // get functions
        std::string get_name() const;
        scheduling_policy get_scheduling_policy() const;
        std::size_t get_number_pus() const;
        std::vector<size_t> get_pus() const;

        // mechanism for adding resources
        void add_resource(std::size_t pu_number);

        void print_me();

    private:
        std::string pool_name_;
        scheduling_policy scheduling_policy_;
        std::vector<std::size_t> my_pus_;
    };

    class HPX_EXPORT resource_partitioner{
    public:

/*        static resource_partitioner& get_resource_partitioner_(){
            util::static_<resource::resource_partitioner, std::false_type> rp;
            return rp.get();
        }*/

        //! constructor: users shouldn't use the constructor but rather get_resource_partitioner
        resource_partitioner();

        void print_me(); //! used by shoshijak for testing. To be deleted.

        // create a new thread_pool, add it to the RP and return a pointer to it
        void create_thread_pool(std::string name, scheduling_policy sched = scheduling_policy::unspecified);
        void create_default_pool(scheduling_policy sched = scheduling_policy::unspecified);

        //! called in hpx_init run_or_start
        void set_init_affinity_data(hpx::util::command_line_handling cfg);
        void set_default_pool(std::size_t num_threads);
        void set_default_schedulers(std::string queueing);

        //! setup stuff related to pools
        void add_resource(std::size_t resource, std::string pool_name);
        void add_resource_to_default(std::size_t resource);

        //! stuff that has to be done during hpx_init ...
        void set_scheduler(scheduling_policy sched, std::string pool_name);
        void set_threadmanager(threads::threadmanager_base* thrd_manag);
        threads::threadmanager_base* get_thread_manager();

        //! called in hpx_init
        void set_affinity_data(std::size_t num_threads) {
            affinity_data_.set_num_threads(num_threads);
        }

        //! called by constructor of scheduler_base
        threads::policies::detail::affinity_data* get_affinity_data(){
            return &affinity_data_;
        }
        bool default_pool(); // returns whether a default pool already exists

        //! used for handing the thread pool its scheduler. To be written better in the future.
        void set_config(util::command_line_handling cfg){
            cfg_ = &cfg;
        }

        //! called in the threadmanager's constructor to setup the thread pools
        util::command_line_handling* get_config(){
            return cfg_;
        }


        // called in hpx_init
        void init_rp();

        // called in runtime::assign_cores()
        size_t init(threads::policies::init_affinity_data data){
            std::size_t ret = affinity_data_.init(data, topology_);
            thread_manager_->init(data);
            return ret;
        }

        ////////////////////////////////////////////////////////////////////////

        scheduling_policy which_scheduler(std::string pool_name);
        threads::topology& get_topology() const;
        threads::policies::init_affinity_data get_init_affinity_data() const;


    private:

        ////////////////////////////////////////////////////////////////////////

        //! this is ugly, I should probably delete it
        uint64_t get_pool_index(std::string pool_name) const;

        // has to be private bc pointers become invalid after data member thread_pools_ is resized
        // we don't want to allow the user to use it
        init_pool_data* get_pool(std::string pool_name);
        init_pool_data* get_default_pool();

        ////////////////////////////////////////////////////////////////////////

        // counter for instance numbers
        static boost::atomic<int> instance_number_counter_;

        // contains the basic characteristics of the thread pool partitioning ...
        // that will be passed to the runtime
        std::vector<init_pool_data> initial_thread_pools_;

        // pointer to the threadmanager instance
        hpx::threads::threadmanager_base* thread_manager_;

        // reference to the topology
        threads::hwloc_topology_info& topology_;

        // reference to affinity data
        hpx::threads::policies::init_affinity_data init_affinity_data_;
        hpx::threads::policies::detail::affinity_data affinity_data_;

        //! used for handing the thread_pool its scheduler. will have to be written better in the future
        hpx::util::command_line_handling* cfg_;

    };

    } // namespace resource

    static resource::resource_partitioner & get_resource_partitioner()
    {
        util::static_<resource::resource_partitioner, std::false_type> rp;
        return rp.get();
    }


} // namespace hpx



#endif
