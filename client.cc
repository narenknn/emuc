//
// chat_client.cpp
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
#include <thread>
#include <unordered_map>
#include <boost/asio.hpp>
#include "client.hh"

using boost::asio::ip::tcp;

Connection connection;

class SocketClient
{
public:
  static SocketClient* sc;
  SocketClient(boost::asio::io_service& io_service,
      tcp::resolver::iterator endpoint_iterator)
    : io_service_(io_service),
      socket_(io_service)
  {
    do_connect(endpoint_iterator);
    sc = this;
  }

  void write(Consumer *c)
  {
    io_service_.post(
        [this, c]()
        {
          bool write_in_progress = !write_msgs_.empty();
          write_msgs_.emplace_back(c);
          if (!write_in_progress)
          {
            do_write();
          }
        });
  }

  void close()
  {
    io_service_.post([this]() { socket_.close(); });
  }

private:
  void do_connect(tcp::resolver::iterator endpoint_iterator)
  {
    boost::asio::async_connect(socket_, endpoint_iterator,
        [this](boost::system::error_code ec, tcp::resolver::iterator)
        {
          if (!ec)
          {
            do_read_header();
          }
        });
  }

  void do_read_header()
  {
    boost::asio::async_read(socket_,
        boost::asio::buffer((void *)&pipeId, sizeof(pipeId)),
        [this](boost::system::error_code ec, std::size_t /*length*/)
        {
          pipeId = 0;
          auto tranSz = connection.getTranSz(pipeId);
          if ((0 == ec) and (0 != tranSz)) {
            do_read_body(tranSz);
          } else if (ec == boost::asio::error::eof) {
          } else {
            socket_.close();
          }
        });
  }

  void do_read_body(std::uint32_t tranSz)
  {
    boost::asio::async_read(socket_,
        boost::asio::buffer(connection.pipes[pipeId]->_data.get(), tranSz),
        [this](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (0 == ec) {
            do_read_header();
          } else if (ec == boost::asio::error::eof) {
          } else {
            socket_.close();
          }
        });
  }

  void do_write()
  {
    boost::asio::async_write
      (socket_,
       boost::asio::buffer(write_msgs_.front()->getWrPtr(),
                           write_msgs_.front()->getWrPtrSz()),
        [this](boost::system::error_code ec, std::size_t length)
        {
          std::cout << "Wrote length:" << length << "\n";
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
            std::cerr << "SocketClient : Failed during write\n";
          }
        });
  }

private:
  std::uint32_t pipeId;
  boost::asio::io_service& io_service_;
  tcp::socket socket_;
  std::deque<Consumer *> write_msgs_;
};
SocketClient* SocketClient::sc = nullptr;

void
Connection::write(Consumer *c)
{
  if (SocketClient::sc)
    SocketClient::sc->write(c);
}

class tempConsumer : public Consumer
{
public:
  std::unique_ptr<char[]> _data;
  tempConsumer() : Consumer(0, 14), _data{std::make_unique<char[]>(20)} {
    strncpy(_data.get(), "\1\0\0\0cdHello World!1\n", 18);
  }
  char* getWrPtr() { return _data.get(); }
  void receive(char *d)
  {
    std::cout << "Client Obtained: " << std::string{d, 14};
  }
};

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 3)
    {
      std::cerr << "Usage: SocketClient <host> <port>\n";
      return 1;
    }

    boost::asio::io_service io_service;

    tcp::resolver resolver(io_service);
    auto endpoint_iterator = resolver.resolve({ argv[1], argv[2] });
    SocketClient c(io_service, endpoint_iterator);

    std::thread t([&io_service](){ io_service.run(); });

//    char line[message::max_body_length + 1];
//    std::cout << "before cin.getline loop\n";
//    while (std::cin.getline(line, message::max_body_length + 1))
//    {
//      message msg;
//      msg.body_length(std::strlen(line));
//      std::memcpy(msg.body(), line, msg.body_length());
//      msg.encode_header();
//      c.write(msg);
//      std::cout << "in cin.getline loop\n";
//    }

    tempConsumer tc;
    connection.addConsumer(&tc);
    connection.write(&tc);

    c.close();
    t.join();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
