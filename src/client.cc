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
#include "client.hh"

#include "svbit.hh"
#include "shash.hh"

std::unique_ptr<Connection> connection;
std::unique_ptr<PipeConnector> pipe0;

void
Pipe::rawSend(std::shared_ptr<TransBase>& p)
{
  /* local send */
  auto range=connection->pipes.equal_range(pipeId);
  for (auto it=range.first; it!=range.second; ++it) {
    if (this != it->second)
      it->second->receive(p->_data.get());
  }
  /* socket send */
  if (connection->sockPipes.end() != connection->sockPipes.find(pipeId)) {
    if (sc) sc->write(p);
  }
}

Pipe::Pipe(std::uint32_t p, std::uint32_t tz) :
  pipeId(p), tranSz(tz), _recvdata{std::make_unique<char[]>(tz)} {
  connection->addPipe(this);
}

/* Local connection sits in 'pipes'
   All local-local connetion is put into localPipes;
   All local-socket connetion is put into sockPipes;
*/
void
Connection::addPipe(Pipe *p)
{
  if (pipes.end() == pipes.find(p->pipeId)) {
    pipes.emplace(p->pipeId, p);
  }
  /* tell to other end of this pipe */
  auto trans = std::make_shared<TransBase>(sizeof(std::uint32_t)+sizeof(std::uint32_t));
  *(trans->pipeId) = CRC32_STR("//connect//");
  std::memcpy(trans->getWrPtr()+sizeof(std::uint32_t), &(p->pipeId), sizeof(std::uint32_t));
  std::memcpy(trans->getWrPtr()+sizeof(std::uint32_t)+sizeof(std::uint32_t), &(p->tranSz), sizeof(std::uint32_t));
  if (sc) sc->write(trans);
}
void
Connection::addSockPipe(std::uint32_t pi, std::uint32_t sz)
{
  if (sockPipes.end() == sockPipes.find(pi)) {
    sockPipes.emplace(pi, sz);
  }
}

PipeConnector::PipeConnector():
  Pipe{CRC32_STR("//connect//"),
    sizeof(std::uint32_t)+sizeof(std::uint32_t)} {
  connection->addPipe(this);
};

void
PipeConnector::receive(char *d)
{
  std::uint32_t pi, sz;
  std::memcpy(&pi, d, sizeof(pi));
  std::memcpy(&sz, d+sizeof(pi), sizeof(sz));
  /* add socket pipe */
  std::cout << "PipeConnector::receive called pi:" << std::hex << pi << std::dec << " sz:" << sz << "\n";
  connection->addSockPipe(pi, sz);
}

boost::asio::io_service io_service;
tcp::resolver resolver(io_service);
std::unique_ptr<SocketClient> sc
{std::make_unique<SocketClient>
    (io_service, resolver.resolve({"localhost", "9001"})) };

extern "C" void
ClientInit()
{
}
