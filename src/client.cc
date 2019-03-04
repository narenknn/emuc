//
// chat_client.cpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2016 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at <a href="http://www.boost.org/LICENSE_1_0.txt">http://www.boost.org/LICENSE_1_0.txt</a>)
//

#include <queue>
#include <vector>
#include <iostream>
#include <unordered_map>
#include <boost/exception/all.hpp>
#include <boost/throw_exception.hpp>
#include <boost/asio.hpp>
#include "scCommon.hh"
#include "client.hh"

#include "svbit.hh"
#include "shash.hh"

using SS = SocketClient;
#include "scCommon.cc"

boost::asio::io_service io_service;
tcp::resolver resolver(io_service);
std::unique_ptr<SocketClient> ss {std::make_unique<SocketClient>
    (io_service, resolver.resolve({"localhost", "9001"})) };

extern "C" void
pollOnce()
{
  if (ss) ss->serviceLoop();
  io_service.poll();
}
