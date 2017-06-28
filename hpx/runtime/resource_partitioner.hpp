//  Copyright (c)      2017 Shoshana Jakobovits
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_RESOURCE_PARTITIONER)
#define HPX_RESOURCE_PARTITIONER

#include <hpx/runtime/threads/coroutines/detail/coroutine_self.hpp>
#include <hpx/runtime/threads/cpu_mask.hpp>
#include <hpx/runtime/threads/policies/hwloc_topology_info.hpp>
#include <hpx/runtime/threads/policies/topology.hpp>
#include <hpx/runtime/threads/threadmanager.hpp>
#include <hpx/runtime/threads/topology.hpp>
#include <hpx/util/command_line_handling.hpp>
#include <hpx/util/thread_specific_ptr.hpp>
//
#include <hpx/runtime/threads/policies/scheduler_mode.hpp>
#include <hpx/runtime/threads/policies/callback_notifier.hpp>
#include <hpx/runtime/threads/detail/thread_pool.hpp>

#include <boost/atomic.hpp>

#include <vector>
#include <string>
#include <stdexcept>
#include <algorithm>

namespace hpx {
namespace resource
{

    struct core;
    struct numa_domain;

    struct pu {
        std::size_t     id_;
        core           *core_;
        std::vector<pu> pus_sharing_core();
        std::vector<pu> pus_sharing_numa_domain();
    };

    struct core {
        std::size_t       id_;
        numa_domain      *domain_;
        std::vector<pu>   pus_;
        std::vector<core> cores_sharing_numa_domain();
    };

    struct numa_domain {
        std::size_t       id_;
        std::vector<core> cores_;
    };

    using scheduler_function = std::function<
        hpx::threads::detail::thread_pool*(
            hpx::threads::policies::callback_notifier &notifier,
            std::size_t index, char const* name,
            hpx::threads::policies::scheduler_mode m)>;

    // scheduler assigned to thread_pool
    // choose same names as in command-line options except with _ instead of -
    enum scheduling_policy {
        user_defined = -2,
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

        init_pool_data(const std::string &name,
            scheduling_policy = scheduling_policy::unspecified);

        init_pool_data(const std::string &name,
            scheduler_function create_func);

        // set functions
        void set_scheduler(scheduling_policy sched);
        void set_mask(threads::mask_type mask);
        void set_thread_num(std::size_t num_threads);

        // get functions
        const std::string &get_name() const;
        scheduling_policy get_scheduling_policy() const;
        std::size_t get_number_used_pus() const;
        std::size_t get_num_threads() const;
        threads::mask_type get_pus() const;

        // mechanism for adding resources
        // index is zero-based
        void add_resource(std::size_t pu_index);

        void print_pool() const;

        friend class resource_partitioner;

    private:
        std::string         pool_name_;
        scheduling_policy   scheduling_policy_;
        threads::mask_type  assigned_pus_; //! PUs this pool is allowed to run on
        std::size_t         num_threads_;
        scheduler_function  create_function_;
    };

    class HPX_EXPORT resource_partitioner{
    public:

        // constructor: users shouldn't use the constructor
        // but rather get_resource_partitioner
        resource_partitioner(std::size_t num_special_pools_ = 0);

        void print_init_pool_data() const;

        // create a thread_pool
        void create_thread_pool(const std::string &name,
            scheduling_policy sched = scheduling_policy::unspecified);

        // create a thread_pool with a callback function for creating a custom scheduler
        void create_thread_pool(const std::string &name,
            scheduler_function scheduler_creation);

        // Functions to add processing units to thread pools via
        // the pu/core/numa_domain API
        void add_resource(const hpx::resource::pu &p,
            const std::string &pool_name);
        void add_resource(const std::vector<hpx::resource::pu> &pv,
            const std::string &pool_name);
        void add_resource(const hpx::resource::core &c,
            const std::string &pool_name);
        void add_resource(const std::vector<hpx::resource::core> &cv,
            const std::string &pool_name);
        void add_resource(const hpx::resource::numa_domain &nd,
            const std::string &pool_name);
        void add_resource(const std::vector<hpx::resource::numa_domain> &ndv,
            const std::string &pool_name);

        // not sure we need this
        void add_resource_to_default(hpx::resource::pu resource);

        // stuff that has to be done during hpx_init ...
        void set_scheduler(scheduling_policy sched, const std::string &pool_name);
        void set_threadmanager(threads::threadmanager_base* thrd_manag);
        threads::threadmanager_base* get_thread_manager() const;

        // called by constructor of scheduler_base
        threads::policies::detail::affinity_data* get_affinity_data() {
            return &affinity_data_;
        }

        // Does initialization of all resources and internal data of the resource partitioner
        // called in hpx_init
        void init_resources(util::command_line_handling cfg);

        // called in runtime::assign_cores()
        size_t init(threads::policies::init_affinity_data data) {
            std::size_t ret = affinity_data_.init(data, topology_);
            thread_manager_->init();
            return ret;
        }

        ////////////////////////////////////////////////////////////////////////

        scheduling_policy which_scheduler(const std::string &pool_name);
        threads::topology& get_topology() const;
        threads::policies::init_affinity_data get_init_affinity_data() const;
        size_t get_num_pools() const;
        size_t get_num_threads(const std::string &pool_name);
        const std::string &get_pool_name(size_t index) const;

        const scheduler_function &get_pool_creator(size_t index) const;

        const std::vector<numa_domain> &get_numa_domains() {
            return numa_domains_;
        }

    private:

        ////////////////////////////////////////////////////////////////////////

        void fill_topology_vectors();

        ////////////////////////////////////////////////////////////////////////

        // called in hpx_init run_or_start
        void set_init_affinity_data(hpx::util::command_line_handling const& cfg);
        void set_affinity_data(std::size_t num_threads);
        void setup_pools(std::size_t num_threads);
        void set_default_schedulers(const std::string &queueing);
        bool check_oversubscription() const;
        bool check_empty_pools() const;


        //! this is ugly, I should probably delete it
        uint64_t get_pool_index(const std::string &pool_name) const;

        // has to be private bc pointers become invalid after data member thread_pools_ is resized
        // we don't want to allow the user to use it
        init_pool_data* get_pool(const std::string &pool_name);
        init_pool_data* get_default_pool();

        ////////////////////////////////////////////////////////////////////////

        // counter for instance numbers
        static boost::atomic<int> instance_number_counter_;

        // contains the basic characteristics of the thread pool partitioning ...
        // that will be passed to the runtime
        std::vector<init_pool_data> initial_thread_pools_;

        // pointer to the threadmanager instance
        hpx::threads::threadmanager_base* thread_manager_;

        // reference to the topology and affinity data
        threads::hwloc_topology_info& topology_;
        hpx::threads::policies::init_affinity_data init_affinity_data_;
        hpx::threads::policies::detail::affinity_data affinity_data_;

        // flag set by add_resource
        bool set_affinity_from_resource_partitioner_;
        std::vector<numa_domain> numa_domains_;

    };

    } // namespace resource

    static resource::resource_partitioner & get_resource_partitioner()
    {
        util::static_<resource::resource_partitioner, std::false_type> rp;
        return rp.get();
    }


} // namespace hpx



#endif
