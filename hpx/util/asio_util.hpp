//  Copyright (c) 2007-2011 Hartmut Kaiser
//  Copyright (c) 2011      Bryce Lelbach
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_UTIL_ASIOUTIL_MAY_16_2008_1212PM)
#define HPX_UTIL_ASIOUTIL_MAY_16_2008_1212PM

#include <hpx/config.hpp>

#include <boost/fusion/include/vector.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace hpx { namespace util
{
    ///////////////////////////////////////////////////////////////////////////
    bool HPX_EXPORT get_endpoint(std::string const& addr, boost::uint16_t port,
        boost::asio::ip::tcp::endpoint& ep);

    boost::fusion::vector2<boost::uint16_t, boost::uint16_t>
    HPX_EXPORT get_random_ports();

    boost::uint16_t HPX_EXPORT get_random_port();
}}

#endif

