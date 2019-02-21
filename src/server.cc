//
// chat_server.cpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2016 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at <a href="http://www.boost.org/LICENSE_1_0.txt">http://www.boost.org/LICENSE_1_0.txt</a>)
//

#include <deque>
#include <iostream>
#include <unordered_map>
#include <boost/asio.hpp>
#include "server.hh"

Connection connection;

std::shared_ptr<SocketServer> ss;

void
Connection::write(Consumer *c)
{
  if (ss)
    ss->write(c);
}
