//  Copyright (c)      2017 Shoshana Jakobovits
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_RESOURCE_PARTITIONER)
#define HPX_RESOURCE_PARTITIONER

#include <hpx/include/runtime.hpp>
#include <hpx/runtime/threads/topology.hpp>
#include <hpx/runtime/threads/detail/thread_pool.hpp>

#include <vector>
#include <string>

namespace hpx{

    // structure used to encapsulate all characteristics of thread_pools
    // as specified by the user in int main()
    struct initial_thread_pool{
    public:
        initial_thread_pool()
                : pool_name_(),
                  my_pus_(std::vector<std::size_t>(0))
        {}

        initial_thread_pool(std::string name)
                : pool_name_(name),
                  my_pus_(std::vector<std::size_t>(0))
        {}


        // mechanism for adding resources
        void add_resource(std::size_t pu_number){
            my_pus_.push_back(pu_number);
        }

    private:
        std::string pool_name_;
        std::vector<size_t> my_pus_;

    };


    class resource_partitioner{
    public:
        resource_partitioner() // queries hwloc, sets internal parameters
            : initial_thread_pool_(std::vector<initial_thread_pool>(0))
        {
                //! do not allow the creation of more than one RP instance
                //! cf counter for runtime, do the same and add test and exception here

                //! topology is gonna be tricky? see where create_topology is called
                //! probs move it from runtime to RP
        }

        // create a new thread_pool, add it to the RP and return a pointer to it
        initial_thread_pool* create_thread_pool(std::string name){
            initial_thread_pool_.push_back(initial_thread_pool(name));
        }

        // lots of get_functions
/*        std::size_t get_number_pools(){
            return thread_pools_.size();
        }*/

    private:
        ////////////////////////////////////////////////////////////////////////

/*        resource_partitioner* get(){ // for get_ptr function
            return
        }*/


        ////////////////////////////////////////////////////////////////////////

        // contains the basic characteristics
        std::vector<initial_thread_pool> initial_thread_pool_;

        // actual thread pools of OS-threads
//        std::vector<threads::detail::thread_pool> thread_pools_; //! template param?

        // reference to the topology
//        threads::topology& topology_;


        // counter for instance numbers

    };

/*    resource_partitioner * get_resource_partitioner_ptr() const
    {
        //! throw exception if this is called before the RP has been instanciated
        // if resource_partitioner has been instanciated already (most cases)
        resource_partitioner** rp = runtime::runtime_.get();
        return rp ? *rp : nullptr;
    }*/

}



#endif