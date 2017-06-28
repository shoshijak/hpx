//  Copyright (c)      2017 Shoshana Jakobovits
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/runtime/resource_partitioner.hpp>
#include <hpx/runtime/threads/thread_data_fwd.hpp>
#include <hpx/include/runtime.hpp>

namespace hpx { namespace resource {

    ////////////////////////////////////////////////////////////////////////

    init_pool_data::init_pool_data(std::string name, scheduling_policy sched)
        : pool_name_(name),
          scheduling_policy_(sched)
    {
        if(name.empty())
        throw std::invalid_argument("cannot instantiate a initial_thread_pool with empty string as a name.");
    };

    std::string init_pool_data::get_name() const {
        return pool_name_;
    }

    scheduling_policy init_pool_data::get_scheduling_policy() const {
        return scheduling_policy_;
    }

    std::size_t init_pool_data::get_number_pus() const {
        return my_pus_.size();
    }

    std::vector<size_t> init_pool_data::get_pus() const {
        return my_pus_;
    }

    // mechanism for adding resources
    void init_pool_data::add_resource(std::size_t pu_number){
        my_pus_.push_back(pu_number);

        //! throw exception if resource does not exist
        //! or the input parameter is invalid or something like that ...

    }

    void init_pool_data::set_scheduler(scheduling_policy sched){
        scheduling_policy_ = sched;
    }

    ////////////////////////////////////////////////////////////////////////

    resource_partitioner::resource_partitioner()
        : topology_(threads::create_topology())
          /*affinity_data_(static_cast<std::size_t>(0)) //! to be changed!!!*/
    {



        // initialize our TSS
        resource_partitioner::init_tss();

        // allow only one resource_partitioner instance
        if(instance_number_counter_++ >= 0){
            throw std::runtime_error("Cannot instantiate more than one resource partitioner");
        }
    }

    // Constructor called from hpx_init in int run_or_start(...)
    // if the resource partitioner has not been instantiated by the user in int main()
    resource_partitioner::resource_partitioner(
            hpx::threads::policies::init_affinity_data init_affdat,
            std::size_t num_threads, std::string queueing
    )
            : topology_(threads::create_topology()), //! this thing doesn't work but I don't know why ... :(
              init_affinity_data_(init_affdat)
    {
        // initialize our TSS
        resource_partitioner::init_tss();

        // allow only one resource_partitioner instance
        if(instance_number_counter_++ >= 0){
            throw std::runtime_error("Cannot instantiate more than one resource partitioner");
        }

        set_default_pool(num_threads); //! give it default argument specifying the scheduler for the default pool

    }

    // if resource partitioner has not been instantiated yet, it simply returns a nullptr
    resource_partitioner & resource_partitioner::create_resource_partitioner() {
        util::static_<
                resource_partitioner, std::false_type
        > rp;
        return rp.get();
    }

    void resource_partitioner::set_init_affinity_data(hpx::threads::policies::init_affinity_data init_affdat){
        init_affinity_data_ = init_affdat;
    }

    void resource_partitioner::set_default_pool(std::size_t num_threads) {
        //! check whether the user created a default_pool already, and whether this default_pool has a scheduler.
        //! In this case, do nothing
        //! take all non-assigned resources and throw them in a regular default pool
        //! Ugh, how exactly should this interact with num_threads_ specified in cmd line??

    }

    void resource_partitioner::set_default_schedulers(std::string queueing){

        // select the default scheduler
        scheduling_policy default_scheduler;

        if (0 == std::string("local").find(queueing))
        {
            default_scheduler = scheduling_policy::local;
        }
        else if (0 == std::string("local-priority-fifo").find(queueing))
        {
            default_scheduler = scheduling_policy::local_priority_fifo ;
        }
        else if (0 == std::string("local-priority-lifo").find(queueing))
        {
            default_scheduler = scheduling_policy::local_priority_lifo;
        }
        else if (0 == std::string("static").find(queueing))
        {
            default_scheduler = scheduling_policy::static_;
        }
        else if (0 == std::string("static-priority").find(queueing))
        {
            default_scheduler = scheduling_policy::static_priority;
        }
        else if (0 == std::string("abp-priority").find(queueing))
        {
            default_scheduler = scheduling_policy::abp_priority;
        }
        else if (0 == std::string("hierarchy").find(queueing))
        {
            default_scheduler = scheduling_policy::hierarchy;
        }
        else if (0 == std::string("periodic-priority").find(queueing))
        {
            default_scheduler = scheduling_policy::periodic_priority;
        }
        else if (0 == std::string("throttle").find(queueing)) {
            default_scheduler = scheduling_policy::throttle;
        }
        else {
            throw detail::command_line_error(
                    "Bad value for command line option --hpx:queuing");
        }

        // set this scheduler on the pools that do not have a specified scheduler yet
        for(auto itp : initial_thread_pools_){
            if(itp.get_scheduling_policy() == unspecified){
                itp.set_scheduler(default_scheduler);
            }
        }

    }



        // create a new thread_pool, add it to the RP and return a pointer to it
    init_pool_data* resource_partitioner::create_thread_pool(std::string name, scheduling_policy sched)
    {
        if(name.empty())
            throw std::invalid_argument("cannot instantiate a initial_thread_pool with empty string as a name.");

        initial_thread_pools_.push_back(init_pool_data(name, sched));
        init_pool_data* ret(&initial_thread_pools_[initial_thread_pools_.size()-1]);
        return ret;
    }

    void resource_partitioner::add_resource(std::size_t resource, std::string pool_name){
        get_pool(pool_name)->add_resource(resource);
    }

    void resource_partitioner::set_scheduler(scheduling_policy sched, std::string pool_name){
        get_pool(pool_name)->set_scheduler(sched);
    }

    void resource_partitioner::init(){
        //! what do I actually need to do?
    }


    threads::topology& resource_partitioner::get_topology() const
    {
        return topology_;
    }

    threads::policies::init_affinity_data resource_partitioner::get_init_affinity_data() const
    { //! should this return a pointer instead of a copy?
        return init_affinity_data_;
    }




        ////////////////////////////////////////////////////////////////////////

    util::thread_specific_ptr<resource_partitioner*, resource_partitioner::tls_tag> resource_partitioner::resource_partitioner_;

    void resource_partitioner::init_tss()
    {
        // initialize our TSS
        if (nullptr == resource_partitioner::resource_partitioner_.get())
        {
            HPX_ASSERT(nullptr == threads::thread_self::get_self());
            resource_partitioner::resource_partitioner_.reset(new resource_partitioner* (this));
            //!threads::thread_self::init_self();//!
        }
    }

    //! never called ... ?!?
/*    void resource_partitioner::deinit_tss()
    {
        // reset our TSS
        threads::thread_self::reset_self();
        util::reset_held_lock_data();
        threads::reset_continuation_recursion_count();
    }*/


    ////////////////////////////////////////////////////////////////////////

    uint64_t resource_partitioner::get_pool_index(std::string pool_name){
        std::size_t N = initial_thread_pools_.size();
        for(size_t i(0); i<N; i++) {
            if (initial_thread_pools_[i].get_name() == pool_name) {
                return i;
            }
        }

        throw std::invalid_argument(
                "the resource partitioner does not own a thread pool named \"" + pool_name + "\" \n");

    }

    // has to be private bc pointers become invalid after data member thread_pools_ is resized
    // we don't want to allow the user to use it
    init_pool_data* resource_partitioner::get_pool(std::string pool_name){
        auto pool = std::find_if(
                initial_thread_pools_.begin(), initial_thread_pools_.end(),
                [&pool_name](init_pool_data itp) -> bool {return (itp.get_name() == pool_name);}
        );

        if(pool != initial_thread_pools_.end()){
            init_pool_data* ret(&(*pool)); //! ugly
            return ret;
        }

        throw std::invalid_argument(
                "the resource partitioner does not own a thread pool named \"" + pool_name + "\" \n");
    }

    ////////////////////////////////////////////////////////////////////////

    boost::atomic<int> resource_partitioner::instance_number_counter_(-1);


    } // namespace resource

    // if resource partitioner has not been instantiated yet, it simply returns a nullptr
    /*
    resource::resource_partitioner & get_resource_partitioner()
    {
        return resource::resource_partitioner::create_resource_partitioner();

        resource::resource_partitioner::
        if(hpx::get_runtime_ptr() == nullptr){
            //! if the runtime has not yet been instantiated
            resource::resource_partitioner** rp = resource::resource_partitioner::resource_partitioner_.get();
            return rp ? *rp : nullptr;
        } else {
            //! if the runtime already has been instantiated
            return hpx::get_runtime_ptr()->get_resource_partitioner_ptr();
        }


    }*/

} // namespace hpx
