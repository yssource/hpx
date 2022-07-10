//  Copyright (c) 2007-2022 Hartmut Kaiser
//  Copyright (c) 2019 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/futures/future.hpp>

#include <hpx/execution/traits/is_execution_policy.hpp>

#include <type_traits>

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace parallel { namespace util { namespace detail {

    template <typename ExPolicy, template <typename...> class Partitioner,
        template <typename...> class TaskPartitioner, typename Enable = void>
    struct select_partitioner
    {
        template <typename... Args>
        using apply = Partitioner<ExPolicy, Args...>;
    };

    template <typename ExPolicy, template <typename...> class Partitioner,
        template <typename...> class TaskPartitioner>
    struct select_partitioner<ExPolicy, Partitioner, TaskPartitioner,
        std::enable_if_t<hpx::is_async_execution_policy_v<ExPolicy> &&
            !hpx::execution_policy_has_scheduler_executor_v<ExPolicy>>>
    {
        template <typename... Args>
        using apply = TaskPartitioner<ExPolicy, Args...>;
    };
}}}}    // namespace hpx::parallel::util::detail
