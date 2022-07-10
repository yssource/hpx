//  Copyright (c) ETH Zurich 2021
//  Copyright (c) 2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/execution/algorithms/detail/partial_algorithm.hpp>
#include <hpx/execution/algorithms/let_value.hpp>
#include <hpx/execution/algorithms/then.hpp>
#include <hpx/execution/traits/is_execution_policy.hpp>
#include <hpx/execution_base/completion_signatures.hpp>
#include <hpx/execution_base/sender.hpp>
#include <hpx/executors/execution_policy.hpp>

#include <type_traits>
#include <utility>

namespace hpx::detail {

    // This is a lighter-weight alternative to bind for use in parallel
    // algorithm overloads, where one needs to bind an execution policy to an
    // algorithm for use in execution::then. Typically used together with
    // then_with_bound_algorithm.
    template <typename Tag, typename ExPolicy>
    struct bound_algorithm
    {
        std::decay_t<ExPolicy> policy;

        template <typename T1, typename... Ts>
        auto operator()(T1&& t1, Ts&&... ts) -> decltype(Tag{}(
            HPX_MOVE(policy), HPX_FORWARD(T1, t1), HPX_FORWARD(Ts, ts)...))
        {
            return Tag{}(
                HPX_MOVE(policy), HPX_FORWARD(T1, t1), HPX_FORWARD(Ts, ts)...);
        }
    };

    // Detects if the given type is a bound_algorithm.
    template <typename Bound>
    struct is_bound_algorithm : std::false_type
    {
    };

    template <typename Tag, typename ExPolicy>
    struct is_bound_algorithm<bound_algorithm<Tag, ExPolicy>> : std::true_type
    {
    };

    template <typename Bound>
    inline constexpr bool is_bound_algorithm_v =
        is_bound_algorithm<Bound>::value;

    // Helper function for use in creating overloads of parallel algorithms that
    // take senders. Takes an execution policy, a predecessor sender, and an
    // "algorithm" (i.e. a tag) and applies then with the predecessor
    // sender and the execution policy bound to the algorithm.
    template <typename Tag, typename ExPolicy, typename Predecessor>
    decltype(auto) then_with_bound_algorithm(
        Predecessor&& predecessor, ExPolicy&& policy)
    {
        if constexpr (hpx::execution_policy_has_scheduler_executor_v<ExPolicy>)
        {
            // If the executor contained in the execution policy explicitly
            // returns senders, we don't need to wrap the algorithm in any
            // specific way as it directly integrates with the given
            // predecessor.
            //
            // We also string any asynchrony requirements from the given policy
            // as the senders returned by the executor naturally provide this.
            auto non_task_policy = hpx::execution::experimental::to_non_task(
                HPX_FORWARD(ExPolicy, policy));

            return hpx::execution::experimental::let_value(
                HPX_FORWARD(Predecessor, predecessor),
                bound_algorithm<Tag, decltype(non_task_policy)>{
                    HPX_MOVE(non_task_policy)});
        }
        else if constexpr (hpx::execution::detail::has_async_execution_policy_v<
                               ExPolicy>)
        {
            // If the given execution policy has a task policy, i.e. the
            // algorithm can return a future, we use the task policy since we
            // can then directly return the future as a sender and avoid
            // potential blocking that may happen internally.
            auto task_policy = hpx::execution::experimental::to_task(
                HPX_FORWARD(ExPolicy, policy));

            return hpx::execution::experimental::let_value(
                HPX_FORWARD(Predecessor, predecessor),
                bound_algorithm<Tag, decltype(task_policy)>{
                    HPX_MOVE(task_policy)});
        }
        else
        {
            // If the policy does not have a task policy, the algorithm can only
            // be called synchronously. In this case we only use then to chain
            // the algorithm after the predecessor sender.
            return hpx::execution::experimental::then(
                HPX_FORWARD(Predecessor, predecessor),
                bound_algorithm<Tag, ExPolicy>{HPX_FORWARD(ExPolicy, policy)});
        }
    }

    // Helper base class for implementing parallel algorithm DPOs. See
    // tag_fallback documentation for details. Compared to tag_fallback this
    // adds two tag_fallback_invoke overloads that are generic for all
    // parallel algorithms:
    //
    //   1. An overload taking a predecessor sender which sends all arguments
    //      for the regular parallel algorithm overload, except an execution
    //      policy; and an execution policy.
    //   2. An overload taking only an execution policy. This overload returns a
    //      partially applied parallel algorithm, and needs to be supplied a
    //      predecessor sender. The partially applied algorithm is callable with
    //      a predecessor sender: partially_applied_algorithm(predecessor). The
    //      predecessor can also be supplied  using the operator| overload:
    //      predecessor | partially_applied_parallel_algorithm.
    template <typename Tag>
    struct tag_parallel_algorithm : hpx::functional::detail::tag_fallback<Tag>
    {
        // clang-format off
        template <typename Predecessor, typename ExPolicy,
            HPX_CONCEPT_REQUIRES_(
                hpx::is_execution_policy_v<ExPolicy> &&
               !detail::is_bound_algorithm_v<Predecessor> &&
                hpx::execution::experimental::is_sender_v<
                    std::decay_t<Predecessor>>
            )>
        // clang-format on
        friend auto tag_fallback_invoke(
            Tag, Predecessor&& predecessor, ExPolicy&& policy)
        {
            return detail::then_with_bound_algorithm<Tag>(
                HPX_FORWARD(Predecessor, predecessor),
                HPX_FORWARD(ExPolicy, policy));
        }

        // clang-format off
        template <typename ExPolicy,
            HPX_CONCEPT_REQUIRES_(
                hpx::is_execution_policy_v<ExPolicy>
            )>
        // clang-format on
        friend auto tag_fallback_invoke(Tag, ExPolicy&& policy)
        {
            return hpx::execution::experimental::detail::partial_algorithm<Tag,
                ExPolicy>{HPX_FORWARD(ExPolicy, policy)};
        }
    };
}    // namespace hpx::detail
