#pragma once

#define EMUC_SERVER_HH

#ifdef EMUC_CLIENT_HH
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
    std::cout << "in Pipe::receive()\n";
    for (auto& consumer: _consumers)
      consumer->receive(_data.get());
  }

  Pipe(std::uint32_t p, std::uint32_t tz) :
    pipeId(p), tranSz(tz), _data{std::make_unique<char[]>(tz)} {
  }
};

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

//----------------------------------------------------------------------
class SocketServer : public std::enable_shared_from_this<SocketServer>
{
public:
  SocketServer(tcp::socket socket)
    : socket_(std::move(socket))
  {
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
extern std::shared_ptr<SocketServer> ss;

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
            ss = std::make_shared<SocketServer>(std::move(socket_));
            ss->start();
          }

          do_accept();
        });
  }

  tcp::acceptor acceptor_;
  tcp::socket socket_;
};
