//
// chat_server.cpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2016 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at <a href="http://www.boost.org/LICENSE_1_0.txt">http://www.boost.org/LICENSE_1_0.txt</a>)
//

#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include <utility>
#include <unordered_map>
#include <boost/asio.hpp>

#include "server.hh"

using boost::asio::ip::tcp;

Connection connection;

//----------------------------------------------------------------------
class SocketServer : public std::enable_shared_from_this<SocketServer>
{
public:
  static SocketServer* ss;
  SocketServer(tcp::socket socket)
    : socket_(std::move(socket))
  {
    ss = this;
  }

  void start()
  {
    do_read_header();
  }

  void write(Consumer *c)
  {
    bool write_in_progress = !write_msgs_.empty();
    write_msgs_.emplace_back(c);
    if (!write_in_progress)
    {
      do_write();
    }
  }

private:
  void do_read_header()
  {
    auto self(shared_from_this());
    boost::asio::async_read(socket_,
        boost::asio::buffer((void *)&pipeId, sizeof(pipeId)),
        [this, self](boost::system::error_code ec, std::size_t length)
        {
          std::cout << "pipeId obtained was :" << pipeId << " ec:" << ec << "\n";
          pipeId = 0;
          auto tranSz = connection.getTranSz(pipeId);
          if ((0 == ec) and (0 != tranSz)) {
              do_read_body(tranSz);
          } else if (ec == boost::asio::error::eof) {
          } else {
            std::cerr << "Error while Read Header ec(" << ec << "), sizeof(pipeId)(" << sizeof(pipeId) << ") size(" << tranSz << ") length(" << length << ")\n";
          }
        });
  }

  void do_read_body(std::uint32_t tranSz)
  {
    auto self(shared_from_this());
    boost::asio::async_read(socket_,
        boost::asio::buffer(connection.pipes[pipeId]->_data.get(), tranSz),
        [this, self](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (0 == ec) {
            connection.pipes[pipeId]->receive();
            do_read_header();
          } else if (ec == boost::asio::error::eof) {
          } else {
            std::cerr << "Error while Read Body ec(" << ec << "), pipeId(" << pipeId << ")\n";
          }
        });
  }

  void do_write()
  {
    auto self(shared_from_this());
    boost::asio::async_write
      (socket_,
       boost::asio::buffer(write_msgs_.front()->getWrPtr(),
                           write_msgs_.front()->getWrPtrSz()),
        [this, self](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec)
          {
            write_msgs_.pop_front();
            if (!write_msgs_.empty())
            {
              do_write();
            }
          }
          else
          {
            std::cerr << "Error while Write Body ec(" << ec << "), pipeId(" << pipeId << ")\n";
          }
        });
  }

  std::uint32_t pipeId;
  tcp::socket socket_;
  std::deque<Consumer *> write_msgs_;
};
SocketServer* SocketServer::ss = nullptr;

void
Connection::write(Consumer *c)
{
  if (SocketServer::ss)
    SocketServer::ss->write(c);
}

//----------------------------------------------------------------------
class Server
{
public:
  Server(boost::asio::io_service& io_service,
      const tcp::endpoint& endpoint)
    : acceptor_(io_service, endpoint),
      socket_(io_service)
  {
    do_accept();
  }

private:
  void do_accept()
  {
    acceptor_.async_accept(socket_,
        [this](boost::system::error_code ec)
        {
          if (!ec)
          {
            std::make_shared<SocketServer>(std::move(socket_))->start();
          }

          do_accept();
        });
  }

  tcp::acceptor acceptor_;
  tcp::socket socket_;
};

//----------------------------------------------------------------------

class tempConsumer : public Consumer
{
public:
  std::unique_ptr<char[]> _data;
  tempConsumer() : Consumer(0, 14), _data{std::make_unique<char[]>(20)} {
    strncpy(_data.get(), "abHello World!1\n", 16);
  }
  char* getWrPtr() { return _data.get(); }
  void receive(char *d)
  {
    std::cout << "Server Obtained: " << std::string{d, 14} << "\n";
  }
};

int main(int argc, char* argv[])
{
  try
  {
    if (argc < 2)
    {
      std::cerr << "Usage: server <port> [<port> ...]\n";
      return 1;
    }

    boost::asio::io_service io_service;

    std::list<Server> servers;
    for (int i = 1; i < argc; ++i)
    {
      tcp::endpoint endpoint(tcp::v4(), std::atoi(argv[i]));
      servers.emplace_back(io_service, endpoint);
    }

    tempConsumer t;
    connection.addConsumer(&t);
    io_service.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
