//  Copyright (c) 2007-2017 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/include/parallel_executors.hpp>
#include <hpx/testing.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <iterator>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

using namespace std::chrono;

///////////////////////////////////////////////////////////////////////////////
hpx::thread::id sync_test(int passed_through)
{
    HPX_TEST_EQ(passed_through, 42);
    return hpx::this_thread::get_id();
}

void apply_test(
    hpx::lcos::local::latch& l, hpx::thread::id& id, int passed_through)
{
    HPX_TEST_EQ(passed_through, 42);
    id = hpx::this_thread::get_id();
    l.count_down(1);
}

///////////////////////////////////////////////////////////////////////////////
template <typename Executor>
void test_timed_apply(Executor& exec)
{
    {
        hpx::lcos::local::latch l(2);
        hpx::thread::id id;

        hpx::parallel::execution::timed_executor<Executor> timed_exec(
            exec, milliseconds(10));

        hpx::parallel::execution::post(
            timed_exec, &apply_test, std::ref(l), std::ref(id), 42);

        l.count_down_and_wait();

        HPX_TEST_NEQ(id, hpx::this_thread::get_id());
    }

    {
        hpx::lcos::local::latch l(2);
        hpx::thread::id id;

        hpx::parallel::execution::timed_executor<Executor> timed_exec(
            exec, steady_clock::now() + milliseconds(10));

        hpx::parallel::execution::post(
            timed_exec, &apply_test, std::ref(l), std::ref(id), 42);

        l.count_down_and_wait();

        HPX_TEST_NEQ(id, hpx::this_thread::get_id());
    }
}

template <typename Executor>
void test_timed_sync(Executor& exec)
{
    {
        hpx::parallel::execution::timed_executor<Executor> timed_exec(
            exec, milliseconds(10));

        HPX_TEST(hpx::parallel::execution::sync_execute(
                     timed_exec, &sync_test, 42) != hpx::this_thread::get_id());
    }

    {
        hpx::parallel::execution::timed_executor<Executor> timed_exec(
            exec, steady_clock::now() + milliseconds(10));

        HPX_TEST(hpx::parallel::execution::sync_execute(
                     timed_exec, &sync_test, 42) != hpx::this_thread::get_id());
    }
}

template <typename Executor>
void test_timed_async(Executor& exec)
{
    {
        hpx::parallel::execution::timed_executor<Executor> timed_exec(
            exec, milliseconds(10));

        HPX_TEST(
            hpx::parallel::execution::async_execute(timed_exec, &sync_test, 42)
                .get() != hpx::this_thread::get_id());
    }

    {
        hpx::parallel::execution::timed_executor<Executor> timed_exec(
            exec, steady_clock::now() + milliseconds(10));

        HPX_TEST(
            hpx::parallel::execution::async_execute(timed_exec, &sync_test, 42)
                .get() != hpx::this_thread::get_id());
    }
}

std::atomic<std::size_t> count_sync(0);
std::atomic<std::size_t> count_apply(0);
std::atomic<std::size_t> count_async(0);
std::atomic<std::size_t> count_sync_at(0);
std::atomic<std::size_t> count_apply_at(0);
std::atomic<std::size_t> count_async_at(0);

template <typename Executor>
void test_timed_executor(std::array<std::size_t, 6> expected)
{
    typedef typename hpx::traits::executor_execution_category<Executor>::type
        execution_category;

    HPX_TEST((std::is_same<hpx::parallel::execution::parallel_execution_tag,
        execution_category>::value));

    count_sync.store(0);
    count_apply.store(0);
    count_async.store(0);
    count_sync_at.store(0);
    count_apply_at.store(0);
    count_async_at.store(0);

    Executor exec;

    test_timed_apply(exec);
    test_timed_sync(exec);
    test_timed_async(exec);

    HPX_TEST_EQ(expected[0], count_sync.load());
    HPX_TEST_EQ(expected[1], count_apply.load());
    HPX_TEST_EQ(expected[2], count_async.load());
    HPX_TEST_EQ(expected[3], count_sync_at.load());
    HPX_TEST_EQ(expected[4], count_apply_at.load());
    HPX_TEST_EQ(expected[5], count_async_at.load());
}

///////////////////////////////////////////////////////////////////////////////
struct test_async_executor1
{
    typedef hpx::parallel::execution::parallel_execution_tag execution_category;

    template <typename F, typename... Ts>
    static hpx::future<
        typename hpx::util::detail::invoke_deferred_result<F, Ts...>::type>
    async_execute(F&& f, Ts&&... ts)
    {
        ++count_async;
        return hpx::async(
            hpx::launch::async, std::forward<F>(f), std::forward<Ts>(ts)...);
    }
};

struct test_timed_async_executor1 : test_async_executor1
{
    template <typename F, typename... Ts>
    static hpx::future<
        typename hpx::util::detail::invoke_deferred_result<F, Ts...>::type>
    async_execute_at(
        hpx::util::steady_time_point const& abs_time, F&& f, Ts&&... ts)
    {
        ++count_async_at;
        hpx::this_thread::sleep_until(abs_time);
        return hpx::async(
            hpx::launch::async, std::forward<F>(f), std::forward<Ts>(ts)...);
    }
};

namespace hpx { namespace parallel { namespace execution {
    template <>
    struct is_two_way_executor<test_async_executor1> : std::true_type
    {
    };

    template <>
    struct is_two_way_executor<test_timed_async_executor1> : std::true_type
    {
    };
}}}    // namespace hpx::parallel::execution

struct test_timed_async_executor2 : test_async_executor1
{
    template <typename F, typename... Ts>
    static typename hpx::util::detail::invoke_deferred_result<F, Ts...>::type
    sync_execute(F&& f, Ts&&... ts)
    {
        ++count_sync;
        return hpx::async(
            hpx::launch::async, std::forward<F>(f), std::forward<Ts>(ts)...)
            .get();
    }
};

struct test_timed_async_executor3 : test_timed_async_executor2
{
    template <typename F, typename... Ts>
    static typename hpx::util::detail::invoke_deferred_result<F, Ts...>::type
    sync_execute_at(
        hpx::util::steady_time_point const& abs_time, F&& f, Ts&&... ts)
    {
        ++count_sync_at;
        hpx::this_thread::sleep_until(abs_time);
        return hpx::async(
            hpx::launch::async, std::forward<F>(f), std::forward<Ts>(ts)...)
            .get();
    }
};

namespace hpx { namespace parallel { namespace execution {
    template <>
    struct is_two_way_executor<test_timed_async_executor2> : std::true_type
    {
    };

    template <>
    struct is_two_way_executor<test_timed_async_executor3> : std::true_type
    {
    };
}}}    // namespace hpx::parallel::execution

struct test_timed_async_executor4 : test_async_executor1
{
    template <typename F, typename... Ts>
    static void post(F&& f, Ts&&... ts)
    {
        ++count_apply;
        hpx::apply(std::forward<F>(f), std::forward<Ts>(ts)...);
    }
};

struct test_timed_async_executor5 : test_timed_async_executor4
{
    template <typename F, typename... Ts>
    static void post_at(
        hpx::util::steady_time_point const& abs_time, F&& f, Ts&&... ts)
    {
        ++count_apply_at;
        hpx::this_thread::sleep_until(abs_time);
        hpx::apply(std::forward<F>(f), std::forward<Ts>(ts)...);
    }
};

namespace hpx { namespace parallel { namespace execution {
    template <>
    struct is_two_way_executor<test_timed_async_executor4> : std::true_type
    {
    };

    template <>
    struct is_two_way_executor<test_timed_async_executor5> : std::true_type
    {
    };
}}}    // namespace hpx::parallel::execution

///////////////////////////////////////////////////////////////////////////////
int hpx_main(int argc, char* argv[])
{
    test_timed_executor<test_async_executor1>({{0, 0, 6, 0, 0, 0}});
    test_timed_executor<test_timed_async_executor1>({{0, 0, 4, 0, 0, 2}});
    test_timed_executor<test_timed_async_executor2>({{2, 0, 4, 0, 0, 0}});
    test_timed_executor<test_timed_async_executor3>({{0, 0, 4, 2, 0, 0}});
    test_timed_executor<test_timed_async_executor4>({{0, 2, 4, 0, 0, 0}});
    test_timed_executor<test_timed_async_executor5>({{0, 0, 4, 0, 2, 0}});

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    // By default this test should run on all available cores
    std::vector<std::string> const cfg = {"hpx.os_threads=all"};

    // Initialize and run HPX
    HPX_TEST_EQ_MSG(
        hpx::init(argc, argv, cfg), 0, "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
