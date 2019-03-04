//
// chat_server.cpp
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
#include "server.hh"

#include "svbit.hh"
#include "shash.hh"

using SS = SocketServer;
#include "scCommon.cc"

boost::asio::io_service io_service;
std::unique_ptr<Server> serverp {std::make_unique<Server>(io_service, tcp::endpoint{tcp::v4(), 9001})};
std::shared_ptr<SocketServer> ss;

extern "C" void
pollOnce()
{
  std::cout << "pollOnce called\n";
  if (ss) ss->serviceLoop();
  io_service.poll();
}

extern "C" void
pollInit()
{
  pipe0.connect();
}
