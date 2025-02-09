//  Copyright (c) 2012 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx.hpp>
#include <hpx/hpx_main.hpp>

///////////////////////////////////////////////////////////////////////////////
bool thrown_exception = false;

void throw_hpx_exception()
{
    thrown_exception = true;
    HPX_THROW_EXCEPTION(
        hpx::bad_request, "throw_hpx_exception", "testing HPX exception");
}

HPX_PLAIN_ACTION(throw_hpx_exception, throw_hpx_exception_action)

///////////////////////////////////////////////////////////////////////////////
int main()
{
    throw_hpx_exception_action act;
    act(hpx::find_here());
    return 0;
}
#endif
