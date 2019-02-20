#pragma once

#define EMUC_CLIENT_HH

#ifdef EMUC_SERVER_HH
#error "Include either server.hh or client.hh!"
#endif

//----------------------------------------------------------------------
using boost::asio::ip::tcp;
class Connection;

//----------------------------------------------------------------------
class Consumer
{
public:
  std::uint32_t pipeId, tranSz;
  virtual char* getWrPtr() = 0;
  virtual std::uint32_t getWrPtrSz() { return sizeof(pipeId) + tranSz; }
  virtual void receive(char *d) = 0;
  Consumer(std::uint32_t p, std::uint32_t tsz) :
    pipeId(p), tranSz(tsz)
  {}
};

//----------------------------------------------------------------------
class Pipe
{
public:
  std::uint32_t pipeId, tranSz;
  std::unique_ptr<char[]> _data;
  std::vector<Consumer *> _consumers;
  void addConsumer(Consumer* c)
  {
    _consumers.emplace_back(c);
  }

  void receive()
  {
    for (auto& consumer: _consumers)
      consumer->receive(_data.get());
  }

  Pipe(std::uint32_t p, std::uint32_t tz) :
    pipeId(p), tranSz(tz), _data{std::make_unique<char[]>(tz)} {
  }
};

//----------------------------------------------------------------------
class Connection
{
public:
  std::unordered_map<std::uint32_t, std::unique_ptr<Pipe>> pipes;
  void addConsumer(Consumer* c)
  {
    if (pipes.end() == pipes.find(c->pipeId))
      pipes.emplace(c->pipeId, std::make_unique<Pipe>(c->pipeId, c->tranSz));
    pipes[c->pipeId]->addConsumer(c);
  }
  std::uint32_t getTranSz(std::uint32_t pipeId)
  {
    if (pipes.end() == pipes.find(pipeId))
      return 0;
    return pipes[pipeId]->tranSz;
  }
  void write(Consumer *c);
};

extern Connection connection;

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
