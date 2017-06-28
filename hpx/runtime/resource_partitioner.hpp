//  Copyright (c)      2017 Shoshana Jakobovits
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_RESOURCE_PARTITIONER)
#define HPX_RESOURCE_PARTITIONER

#include <hpx/runtime/threads/topology.hpp>
#include <hpx/runtime/threads/policies/topology.hpp>
#include <hpx/runtime/threads/policies/hwloc_topology_info.hpp>
#include <hpx/util/thread_specific_ptr.hpp>
#include <hpx/runtime/threads/coroutines/detail/coroutine_self.hpp>

//#include <hpx/runtime/threads/detail/thread_pool.hpp>
//#include <hpx/include/runtime.hpp>

#include <boost/atomic.hpp>

#include <vector>
#include <string>
#include <stdexcept>
#include <algorithm>

namespace hpx { namespace resource {

    // structure used to encapsulate all characteristics of thread_pools
    // as specified by the user in int main()
    class initial_thread_pool{
    public:

        initial_thread_pool(std::string name);

        //! another constructor with size param in case the user already knows at cstrction how many resources will be allocated?

        // get functions
        std::string get_name();

        std::size_t get_number_pus();

        std::vector<size_t> get_pus();

        // mechanism for adding resources
        void add_resource(std::size_t pu_number);

    private:
        std::string pool_name_;
        std::vector<std::size_t> my_pus_;
        //! does it need to hold the information "run HPX on me/not"? ie "can be used for runtime"/not?
        //! would make life easier for ppl who want to run HPX side-by-sie with OpenMP for example?

    };

    class HPX_EXPORT resource_partitioner{
    public:

        resource_partitioner();

        //! used to make a global accessible pointer. Copied from runtime.hpp
        // the TSS holds a pointer to the runtime associated with a given OS thread
        struct tls_tag {};
        static util::thread_specific_ptr<resource_partitioner*, tls_tag> resource_partitioner_;

        //! additional constructors with a bunch of strings, in case I know my names already

        // create a new thread_pool, add it to the RP and return a pointer to it
        initial_thread_pool* create_thread_pool(std::string name);

        void add_resource(std::size_t resource, std::string pool_name);

        // lots of get_functions
/*        std::size_t get_number_pools(){
            return thread_pools_.size();
        }*/

        threads::topology& get_topology() const;
        std::size_t dummy_access() const; //! delete this in the future


    private:
        ////////////////////////////////////////////////////////////////////////

        //! this is ugly, I should probably delete it
        uint64_t get_pool_index(std::string pool_name);

        // has to be private bc pointers become invalid after data member thread_pools_ is resized
        // we don't want to allow the user to use it
        initial_thread_pool* get_pool(std::string pool_name);

        void init_tss();
/*        void deinit_tss();*/ //! is this even needed ? probs should delete this

        ////////////////////////////////////////////////////////////////////////

        // counter for instance numbers
        static boost::atomic<int> instance_number_counter_;

        // pointer to global unique instance of resource_partitioner
        static resource_partitioner* resource_partitioner_ptr;

        // contains the basic characteristics of the thread pool partitioning ...
        // that will be passed to the runtime
        std::vector<initial_thread_pool> initial_thread_pool_;

        // actual thread pools of OS-threads
//        std::vector<threads::detail::thread_pool> thread_pools_; //! template param needed? owned via thread_manager? Different data structure?

        // list of schedulers or is it enough if they're owned by thread_pool?

        // reference to the topology
        threads::hwloc_topology_info& topology_;

        // reference to affinity data
        //! I'll probably have to take this away from runtime

    };

    } // namespace resource

    HPX_API_EXPORT resource::resource_partitioner* get_resource_partitioner_ptr();

} // namespace hpx



#endif
