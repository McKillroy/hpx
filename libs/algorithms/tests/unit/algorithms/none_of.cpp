//  Copyright (c) 2014-2017 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/include/parallel_all_any_none_of.hpp>
#include <hpx/parallel/util/projection_identity.hpp>
#include <hpx/testing.hpp>

#include <cstddef>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#include "test_utils.hpp"

///////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename IteratorTag,
    typename Proj = hpx::parallel::util::projection_identity>
void test_none_of(ExPolicy policy, IteratorTag, Proj proj = Proj())
{
    static_assert(
        hpx::parallel::execution::is_execution_policy<ExPolicy>::value,
        "hpx::parallel::execution::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::size_t iseq[] = {0, 1, 3};
    for (std::size_t i : iseq)
    {
        std::vector<std::size_t> c = test::fill_all_any_none(3, i);    //-V106

        bool result = hpx::parallel::none_of(
            policy, iterator(std::begin(c)), iterator(std::end(c)),
            [](std::size_t v) { return v != 0; }, proj);

        // verify values
        bool expected = std::none_of(std::begin(c), std::end(c),
            [proj](std::size_t v) { return proj(v) != 0; });

        HPX_TEST_EQ(result, expected);
    }
}

template <typename ExPolicy, typename IteratorTag,
    typename Proj = hpx::parallel::util::projection_identity>
void test_none_of_async(ExPolicy p, IteratorTag, Proj proj = Proj())
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::size_t iseq[] = {0, 23, 10007};
    for (std::size_t i : iseq)
    {
        std::vector<std::size_t> c =
            test::fill_all_any_none(10007, i);    //-V106

        hpx::future<bool> f = hpx::parallel::none_of(
            p, iterator(std::begin(c)), iterator(std::end(c)),
            [](std::size_t v) { return v != 0; }, proj);
        f.wait();

        // verify values
        bool expected = std::none_of(std::begin(c), std::end(c),
            [proj](std::size_t v) { return proj(v) != 0; });

        HPX_TEST_EQ(expected, f.get());
    }
}

template <typename IteratorTag>
void test_none_of()
{
    struct proj
    {
        //This projection should cause tests to fail if it is not applied
        //because it causes predicate to evaluate the opposite
        constexpr std::size_t operator()(std::size_t x) const
        {
            return !static_cast<bool>(x);
        }
    };
    using namespace hpx::parallel;

    test_none_of(execution::seq, IteratorTag());
    test_none_of(execution::par, IteratorTag());
    test_none_of(execution::par_unseq, IteratorTag());

    test_none_of(execution::seq, IteratorTag(), proj());
    test_none_of(execution::par, IteratorTag(), proj());
    test_none_of(execution::par_unseq, IteratorTag(), proj());

    test_none_of_async(execution::seq(execution::task), IteratorTag());
    test_none_of_async(execution::par(execution::task), IteratorTag());

    test_none_of_async(execution::seq(execution::task), IteratorTag(), proj());
    test_none_of_async(execution::par(execution::task), IteratorTag(), proj());
}

// template <typename IteratorTag>
// void test_none_of_exec()
// {
//     using namespace hpx::parallel;
//
//     {
//         hpx::threads::executors::local_priority_queue_executor exec;
//         test_none_of(execution::par(exec), IteratorTag());
//     }
//     {
//         hpx::threads::executors::local_priority_queue_executor exec;
//         test_none_of(task(exec), IteratorTag());
//     }
//
//     {
//         hpx::threads::executors::local_priority_queue_executor exec;
//         test_none_of(execution_policy(execution::par(exec)), IteratorTag());
//     }
//     {
//         hpx::threads::executors::local_priority_queue_executor exec;
//         test_none_of(execution_policy(task(exec)), IteratorTag());
//     }
// }

void none_of_test()
{
    test_none_of<std::random_access_iterator_tag>();
    test_none_of<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename IteratorTag>
void test_none_of_exception(ExPolicy policy, IteratorTag)
{
    static_assert(
        hpx::parallel::execution::is_execution_policy<ExPolicy>::value,
        "hpx::parallel::execution::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::size_t iseq[] = {0, 23, 10007};
    for (std::size_t i : iseq)
    {
        std::vector<std::size_t> c =
            test::fill_all_any_none(10007, i);    //-V106

        bool caught_exception = false;
        try
        {
            hpx::parallel::none_of(policy, iterator(std::begin(c)),
                iterator(std::end(c)), [](std::size_t v) {
                    return throw std::runtime_error("test"), v != 0;
                });

            HPX_TEST(false);
        }
        catch (hpx::exception_list const& e)
        {
            caught_exception = true;
            test::test_num_exceptions<ExPolicy, IteratorTag>::call(policy, e);
        }
        catch (...)
        {
            HPX_TEST(false);
        }

        HPX_TEST(caught_exception);
    }
}

template <typename ExPolicy, typename IteratorTag>
void test_none_of_exception_async(ExPolicy p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::size_t iseq[] = {0, 23, 10007};
    for (std::size_t i : iseq)
    {
        std::vector<std::size_t> c =
            test::fill_all_any_none(10007, i);    //-V106

        bool caught_exception = false;
        bool returned_from_algorithm = false;
        try
        {
            hpx::future<void> f =
                hpx::parallel::none_of(p, iterator(std::begin(c)),
                    iterator(std::end(c)), [](std::size_t v) {
                        return throw std::runtime_error("test"), v != 0;
                    });
            returned_from_algorithm = true;
            f.get();

            HPX_TEST(false);
        }
        catch (hpx::exception_list const& e)
        {
            caught_exception = true;
            test::test_num_exceptions<ExPolicy, IteratorTag>::call(p, e);
        }
        catch (...)
        {
            HPX_TEST(false);
        }

        HPX_TEST(caught_exception);
        HPX_TEST(returned_from_algorithm);
    }
}

template <typename IteratorTag>
void test_none_of_exception()
{
    using namespace hpx::parallel;

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_none_of_exception(execution::seq, IteratorTag());
    test_none_of_exception(execution::par, IteratorTag());

    test_none_of_exception_async(
        execution::seq(execution::task), IteratorTag());
    test_none_of_exception_async(
        execution::par(execution::task), IteratorTag());
}

void none_of_exception_test()
{
    test_none_of_exception<std::random_access_iterator_tag>();
    test_none_of_exception<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename IteratorTag>
void test_none_of_bad_alloc(ExPolicy policy, IteratorTag)
{
    static_assert(
        hpx::parallel::execution::is_execution_policy<ExPolicy>::value,
        "hpx::parallel::execution::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::size_t iseq[] = {0, 23, 10007};
    for (std::size_t i : iseq)
    {
        std::vector<std::size_t> c =
            test::fill_all_any_none(10007, i);    //-V106

        bool caught_exception = false;
        try
        {
            hpx::parallel::none_of(policy, iterator(std::begin(c)),
                iterator(std::end(c)),
                [](std::size_t v) { return throw std::bad_alloc(), v != 0; });

            HPX_TEST(false);
        }
        catch (std::bad_alloc const&)
        {
            caught_exception = true;
        }
        catch (...)
        {
            HPX_TEST(false);
        }

        HPX_TEST(caught_exception);
    }
}

template <typename ExPolicy, typename IteratorTag>
void test_none_of_bad_alloc_async(ExPolicy p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::size_t iseq[] = {0, 23, 10007};
    for (std::size_t i : iseq)
    {
        std::vector<std::size_t> c =
            test::fill_all_any_none(10007, i);    //-V106

        bool caught_exception = false;
        bool returned_from_algorithm = false;
        try
        {
            hpx::future<void> f = hpx::parallel::none_of(p,
                iterator(std::begin(c)), iterator(std::end(c)),
                [](std::size_t v) { return throw std::bad_alloc(), v != 0; });
            returned_from_algorithm = true;
            f.get();

            HPX_TEST(false);
        }
        catch (std::bad_alloc const&)
        {
            caught_exception = true;
        }
        catch (...)
        {
            HPX_TEST(false);
        }

        HPX_TEST(caught_exception);
        HPX_TEST(returned_from_algorithm);
    }
}

template <typename IteratorTag>
void test_none_of_bad_alloc()
{
    using namespace hpx::parallel;

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_none_of_bad_alloc(execution::seq, IteratorTag());
    test_none_of_bad_alloc(execution::par, IteratorTag());

    test_none_of_bad_alloc_async(
        execution::seq(execution::task), IteratorTag());
    test_none_of_bad_alloc_async(
        execution::par(execution::task), IteratorTag());
}

void none_of_bad_alloc_test()
{
    test_none_of_bad_alloc<std::random_access_iterator_tag>();
    test_none_of_bad_alloc<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);

    none_of_test();
    none_of_exception_test();
    none_of_bad_alloc_test();
    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    // add command line option which controls the random number generator seed
    using namespace hpx::program_options;
    options_description desc_commandline(
        "Usage: " HPX_APPLICATION_STRING " [options]");

    desc_commandline.add_options()("seed,s", value<unsigned int>(),
        "the random number generator seed to use for this run");
    // By default this test should run on all available cores
    std::vector<std::string> const cfg = {"hpx.os_threads=all"};

    // Initialize and run HPX
    HPX_TEST_EQ_MSG(hpx::init(desc_commandline, argc, argv, cfg), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
