//  Copyright (c) 2021-2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/executors/explicit_scheduler_executor.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/datastructures/tuple.hpp>
#include <hpx/execution/algorithms/bulk.hpp>
#include <hpx/execution/algorithms/keep_future.hpp>
#include <hpx/execution/algorithms/make_future.hpp>
#include <hpx/execution/algorithms/start_detached.hpp>
#include <hpx/execution/algorithms/sync_wait.hpp>
#include <hpx/execution/algorithms/then.hpp>
#include <hpx/execution/algorithms/transfer.hpp>
#include <hpx/execution/executors/execution.hpp>
#include <hpx/execution/executors/execution_parameters.hpp>
#include <hpx/execution_base/execution.hpp>
#include <hpx/execution_base/sender.hpp>
#include <hpx/execution_base/traits/is_executor.hpp>
#include <hpx/functional/bind_back.hpp>
#include <hpx/functional/bind_front.hpp>
#include <hpx/functional/deferred_call.hpp>
#include <hpx/functional/invoke_fused.hpp>
#include <hpx/functional/tag_invoke.hpp>

#include <exception>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::execution::experimental {

    ///////////////////////////////////////////////////////////////////////////
    // A explicit_scheduler_executor wraps any P2300 scheduler and implements
    // the executor functionalities for those. All scheduling functions return
    // senders.
    template <typename BaseScheduler>
    struct explicit_scheduler_executor
    {
        static_assert(hpx::execution::experimental::is_scheduler_v<
                          std::decay_t<BaseScheduler>>,
            "explicit_scheduler_executor requires a scheduler");

        constexpr explicit_scheduler_executor() = default;

        // clang-format off
        template <typename Scheduler,
            typename Enable =
                std::enable_if_t<
                   !std::is_same_v<
                        std::decay_t<Scheduler>, explicit_scheduler_executor> &&
                    hpx::execution::experimental::is_scheduler_v<Scheduler>>>
        // clang-format on
        constexpr explicit explicit_scheduler_executor(Scheduler&& sched)
          : sched_(HPX_FORWARD(Scheduler, sched))
        {
        }

        constexpr explicit_scheduler_executor(
            explicit_scheduler_executor&&) = default;
        constexpr explicit_scheduler_executor& operator=(
            explicit_scheduler_executor&&) = default;
        constexpr explicit_scheduler_executor(
            explicit_scheduler_executor const&) = default;
        constexpr explicit_scheduler_executor& operator=(
            explicit_scheduler_executor const&) = default;

        /// \cond NOINTERNAL
        constexpr bool operator==(
            explicit_scheduler_executor const& rhs) const noexcept
        {
            return sched_ == rhs.sched_;
        }

        constexpr bool operator!=(
            explicit_scheduler_executor const& rhs) const noexcept
        {
            return sched_ != rhs.sched_;
        }

        constexpr auto const& context() const noexcept
        {
            return *this;
        }

        // support all properties exposed by the wrapped scheduler
        template <typename Tag, typename Property,
            typename Enable = std::enable_if_t<hpx::functional::
                    is_tag_invocable_v<Tag, BaseScheduler, Property>>>
        friend explicit_scheduler_executor tag_invoke(
            Tag, explicit_scheduler_executor const& exec, Property&& prop)
        {
            return explicit_scheduler_executor(hpx::functional::tag_invoke(
                Tag{}, exec.sched_, HPX_FORWARD(Property, prop)));
        }

        template <typename Tag,
            typename Enable = std::enable_if_t<
                hpx::functional::is_tag_invocable_v<Tag, BaseScheduler>>>
        friend decltype(auto) tag_invoke(
            Tag, explicit_scheduler_executor const& exec)
        {
            return hpx::functional::tag_invoke(Tag{}, exec.sched_);
        }

        // Associate the parallel_execution_tag executor tag type as a default
        // with this executor.
        using execution_category =
            hpx::traits::executor_execution_category_t<BaseScheduler>;

        // Associate the static_chunk_size executor parameters type as a default
        // with this executor.
        using executor_parameters_type = static_chunk_size;

        // NonBlockingOneWayExecutor interface
        template <typename F, typename... Ts>
        void post(F&& f, Ts&&... ts)
        {
            start_detached(then(schedule(sched_),
                hpx::util::deferred_call(
                    HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...)));
        }

        // OneWayExecutor interface
        template <typename F, typename... Ts>
        auto sync_execute(F&& f, Ts&&... ts)
        {
            return hpx::this_thread::experimental::sync_wait(
                async_execute(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...));
        }

        // TwoWayExecutor interface
        template <typename F, typename... Ts>
        auto async_execute(F&& f, Ts&&... ts)
        {
            return then(schedule(sched_),
                hpx::util::deferred_call(
                    HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...));
        }

        template <typename F, typename Future, typename... Ts>
        auto then_execute(F&& f, Future&& predecessor, Ts&&... ts)
        {
            auto&& predecessor_transfer_sched =
                transfer(keep_future(HPX_FORWARD(Future, predecessor)), sched_);

            return then(HPX_MOVE(predecessor_transfer_sched),
                hpx::bind_back(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...));
        }

        // BulkTwoWayExecutor interface
        template <typename F, typename S, typename... Ts>
        auto bulk_async_execute(F&& f, S const& shape, Ts&&... ts)
        {
            using shape_element =
                typename hpx::traits::range_traits<S>::value_type;
            using result_type = hpx::util::detail::invoke_deferred_result_t<F,
                shape_element, Ts...>;

            static_assert(
                std::is_void_v<result_type>, "std::is_void_v<result_type>");

            return bulk(schedule(sched_), shape,
                hpx::bind_back(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...));
        }

        template <typename F, typename S, typename... Ts>
        void bulk_sync_execute(F&& f, S const& shape, Ts&&... ts)
        {
            hpx::this_thread::experimental::sync_wait(bulk_async_execute(
                HPX_FORWARD(F, f), shape, HPX_FORWARD(Ts, ts)...));
        }

        template <typename F, typename S, typename Future, typename... Ts>
        auto bulk_then_execute(
            F&& f, S const& shape, Future&& predecessor, Ts&&... ts)
        {
            using result_type =
                parallel::execution::detail::then_bulk_function_result_t<F, S,
                    Future, Ts...>;

            static_assert(
                std::is_void_v<result_type>, "std::is_void_v<result_type>");

            auto prereq =
                when_all(keep_future(HPX_FORWARD(Future, predecessor)));

            return bulk(transfer(HPX_MOVE(prereq), sched_), shape,
                hpx::bind_back(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...));
        }

    private:
        std::decay_t<BaseScheduler> sched_;
        /// \endcond
    };

    template <typename BaseScheduler>
    explicit explicit_scheduler_executor(BaseScheduler&& sched)
        -> explicit_scheduler_executor<std::decay_t<BaseScheduler>>;
}    // namespace hpx::execution::experimental

namespace hpx { namespace parallel { namespace execution {

    /// \cond NOINTERNAL
    template <typename BaseScheduler>
    struct is_one_way_executor<hpx::execution::experimental::
            explicit_scheduler_executor<BaseScheduler>> : std::true_type
    {
    };

    template <typename BaseScheduler>
    struct is_never_blocking_one_way_executor<hpx::execution::experimental::
            explicit_scheduler_executor<BaseScheduler>> : std::true_type
    {
    };

    template <typename BaseScheduler>
    struct is_bulk_one_way_executor<hpx::execution::experimental::
            explicit_scheduler_executor<BaseScheduler>> : std::true_type
    {
    };

    template <typename BaseScheduler>
    struct is_two_way_executor<hpx::execution::experimental::
            explicit_scheduler_executor<BaseScheduler>> : std::true_type
    {
    };

    template <typename BaseScheduler>
    struct is_bulk_two_way_executor<hpx::execution::experimental::
            explicit_scheduler_executor<BaseScheduler>> : std::true_type
    {
    };

    template <typename BaseScheduler>
    struct is_scheduler_executor<hpx::execution::experimental::
            explicit_scheduler_executor<BaseScheduler>> : std::true_type
    {
    };
    /// \endcond
}}}    // namespace hpx::parallel::execution
