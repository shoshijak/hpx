//  Copyright (c) 2017 Shoshana Jakobovits
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
//
#include <hpx/parallel/algorithms/for_loop.hpp>
#include <hpx/parallel/executors.hpp>
//
#include <hpx/runtime/resource_partitioner.hpp>
#include <hpx/runtime/threads/cpu_mask.hpp>
#include <hpx/runtime/threads/executors/customized_pool_executors.hpp>
//
// we should not need this
#include <hpx/runtime/threads/detail/thread_pool.hpp>
#include <hpx/runtime/threads/detail/thread_pool_impl.hpp>
#include <hpx/runtime/threads/policies/scheduler_mode.hpp>
//
#include <hpx/include/iostreams.hpp>
#include <hpx/include/runtime.hpp>
//
#include <cmath>
//
#include "shared_priority_scheduler.hpp"
#include "system_characteristics.h"

namespace resource {
namespace pools
{
    enum ids {
        DEFAULT = 0,
        MPI = 1,
        GPU = 2,
        MATRIX = 3,
};
}}

static bool use_pools     = false;
static bool use_scheduler = false;

// this is our custom scheduler type
using high_priority_sched = hpx::threads::policies::shared_priority_scheduler<>;

// Force an instantiation of the pool type templated on our custom scheduler
// we need this to ensure that the pool has the generated member functions needed
// by the linker for this pool type
template class hpx::threads::detail::thread_pool_impl<high_priority_sched>;

// dummy function we will call using async
void do_stuff(std::size_t n){
    std::cout << "[do stuff] " << n << "\n";
    for (std::size_t  i(0); i<n; ++i){
        std::cout << "sin(" << i << ") = " << sin(2*M_PI*i/n) << ", ";
    }
    std::cout << "\n";
}

// this is called on an hpx thread after the runtime starts up
int hpx_main(boost::program_options::variables_map& vm)
{
    if (vm.count("use-pools"))     use_pools = true;
    if (vm.count("use-scheduler")) use_scheduler = true;
    //
    std::cout << "[hpx_main] starting ..."
        << "use_pools " << use_pools << " "
        << "use_scheduler " << use_scheduler << "\n";

    // create an executor with high priority for important tasks
    hpx::threads::executors::default_executor high_priority_executor(
        hpx::threads::thread_priority_critical);
    hpx::threads::executors::default_executor normal_priority_executor;

    hpx::threads::scheduled_executor mpi_executor;
    // create an executor on the mpi pool
    if (use_pools) {
        // get executors
        hpx::threads::executors::customized_pool_executor mpi_exec("mpi");
        mpi_executor = mpi_exec;
        std::cout << "\n[hpx_main] got mpi executor " << std::endl;
    }
    else {
        mpi_executor = high_priority_executor;
    }

    // get a pointer to the resource_partitioner instance
    hpx::resource::resource_partitioner& rpart = hpx::get_resource_partitioner();

    // print partition characteristics
    std::cout << "\n\n[hpx_main] print resource_partitioner characteristics : " << "\n";
    rpart.print_init_pool_data();

    // print partition characteristics
    std::cout << "\n\n[hpx_main] print thread-manager pools : " << "\n";
    hpx::threads::get_thread_manager().print_pools();

    // print system characteristics
    print_system_characteristics();

    // use executor to schedule work on custom pool
    hpx::future<void> future_1 = hpx::async(mpi_executor, &do_stuff, 32);

    auto future_2 = future_1.then(mpi_executor, [](hpx::future<void> &&f) {
        do_stuff(64);
    });
    future_2.get();

    hpx::lcos::local::mutex m;

    int num_threads = hpx::get_num_worker_threads();
    hpx::cout << "HPX using threads = " << num_threads << hpx::endl;
    int loop_count = num_threads*1000;

    std::set<std::thread::id> thread_set;

    // test a parallel algorithm on custom pool with high priority
    hpx::parallel::static_chunk_size fixed(1);
    hpx::parallel::for_loop_strided(
        hpx::parallel::execution::par.with(fixed).on(high_priority_executor), 0, loop_count, 1,
        [&](int i) {
            std::lock_guard<hpx::lcos::local::mutex> lock(m);
            if (thread_set.insert(std::this_thread::get_id()).second) {
                hpx::cout << std::hex << hpx::this_thread::get_id()
                    << " " << std::hex << std::this_thread::get_id()
                    << " high priority i " << std::dec << i << hpx::endl;
            }
        });
    hpx::cout << "thread set contains " << std::dec << thread_set.size() << hpx::endl;
    thread_set.clear();

    // test a parallel algorithm on custom pool with normal priority
    hpx::parallel::for_loop_strided(
        hpx::parallel::execution::par.with(fixed).on(normal_priority_executor), 0, loop_count, 1,
        [&](int i) {
            std::lock_guard<hpx::lcos::local::mutex> lock(m);
            if (thread_set.insert(std::this_thread::get_id()).second) {
                hpx::cout << std::hex << hpx::this_thread::get_id()
                    << " " << std::hex << std::this_thread::get_id()
                    << " normal priority i " << std::dec << i << hpx::endl;
            }
        });
    hpx::cout << "thread set contains " << std::dec << thread_set.size() << hpx::endl;
    thread_set.clear();

    // test a parallel algorithm on mpi_executor
    hpx::parallel::for_loop_strided(
        hpx::parallel::execution::par.with(fixed).on(mpi_executor), 0, loop_count, 1,
        [&](int i) {
            std::lock_guard<hpx::lcos::local::mutex> lock(m);
            if (thread_set.insert(std::this_thread::get_id()).second) {
                hpx::cout << std::hex << hpx::this_thread::get_id()
                    << " " << std::hex << std::this_thread::get_id()
                    << " mpi pool i " << std::dec << i << hpx::endl;
            }
        });
    hpx::cout << "thread set contains " << std::dec << thread_set.size() << hpx::endl;
    thread_set.clear();

    auto high_priority_async_policy = hpx::launch::async_policy(
        hpx::threads::thread_priority_critical);
    auto normal_priority_async_policy = hpx::launch::async_policy();

    // test a parallel algorithm on custom pool with high priority
    hpx::parallel::for_loop_strided(
        hpx::parallel::execution::par.with(high_priority_async_policy).
            with(fixed).on(mpi_executor), 0, loop_count, 1,
        [&](int i) {
            std::lock_guard<hpx::lcos::local::mutex> lock(m);
            if (thread_set.insert(std::this_thread::get_id()).second) {
                hpx::cout << std::hex << hpx::this_thread::get_id()
                    << " " << std::hex << std::this_thread::get_id()
                    << " high priority mpi i " << std::dec << i << hpx::endl;
            }
        });
    hpx::cout << "thread set contains " << std::dec << thread_set.size() << hpx::endl;
    thread_set.clear();

    return hpx::finalize();
}

using namespace boost::program_options;

// the normal int main function that is called at startup and runs on an OS thread
// the user must call hpx::init to start the hpx runtime which will execute hpx_main
// on an hpx thread
int main(int argc, char* argv[])
{
    options_description test_options("Test options");
    test_options.add_options()
            ("use-pools,u", "Enable advanced HPX thread pools and executors")
            ("use-scheduler,s", "Enable custom priority scheduler")
    ;

    options_description desc_cmdline;
    desc_cmdline.add(test_options);

    // HPX uses a boost program options variable map, but we need it before
    // hpx-main, so we will create another one here and throw it away after use
    boost::program_options::variables_map vm;
    boost::program_options::store(command_line_parser(argc, argv)
            .allow_unregistered()
            .options(desc_cmdline)
            .run(), vm);

    if (vm.count("use-pools")) {
      use_pools = true;
    }
    if (vm.count("use-scheduler")) {
      use_scheduler = true;
    }


    auto &rp = hpx::get_resource_partitioner(desc_cmdline, argc, argv);
//    auto &topo = rp.get_topology();
    std::cout << "[main] " << "obtained reference to the resource_partitioner\n";

    // create a thread pool and supply a lambda that returns a new pool with
    // the a user supplied scheduler attached
    rp.create_thread_pool("default", [](
        hpx::threads::policies::callback_notifier &notifier,
        std::size_t index, char const* name,
        hpx::threads::policies::scheduler_mode m)
    {
        std::cout << "User defined scheduler creation callback " << std::endl;
        high_priority_sched::init_parameter_type init(
                    hpx::get_resource_partitioner().get_num_threads(name),
                    "shared-priority-scheduler");
        high_priority_sched* scheduler = new high_priority_sched(init);
        return new hpx::threads::detail::thread_pool_impl<high_priority_sched>(
            scheduler, notifier, index, name, m);
    });

    if (use_pools) {
        // Create a thread pool using the default scheduler provided by HPX
        rp.create_thread_pool("mpi", hpx::resource::scheduling_policy::local_priority_fifo);
        std::cout << "[main] " << "thread_pools created \n";

        rp.add_resource(rp.numa_domains()[0].cores_[0].pus_, "mpi");
        std::cout << "[main] " << "resources added to thread_pools \n";
    }

    return hpx::init(desc_cmdline, argc, argv);
}
