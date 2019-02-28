#pragma once

#define EMUC_CLIENT_HH

#ifdef EMUC_SERVER_HH
#error "Include either server.hh or client.hh!"
#endif

//----------------------------------------------------------------------
using boost::asio::ip::tcp;
class Connection;

//----------------------------------------------------------------------
class TransBase
{
public:
  std::unique_ptr<char[]> _data;
  std::uint32_t *pipeId;
  std::uint32_t tranSz;
  char *getWrPtr() { return _data.get(); }
  std::uint32_t getWrPtrSz() { return sizeof(*pipeId) + tranSz; }
  TransBase(std::uint32_t tsz) :
    tranSz(tsz), _data{std::make_unique<char[]>(tsz+sizeof(*pipeId))},
    pipeId((std::uint32_t *)(_data.get()))
  {
  }
};

//----------------------------------------------------------------------
class Pipe
{
public:
  std::uint32_t pipeId, tranSz;
  std::unique_ptr<char[]> _recvdata;

  void rawSend(std::shared_ptr<TransBase>& p);
  virtual void receive(char *) = 0;

  Pipe(std::uint32_t p, std::uint32_t tz);
};

class PipeConnector : public Pipe
{
public:
  void receive(char *);
  PipeConnector();
};
extern std::unique_ptr<PipeConnector> pipe0;

//----------------------------------------------------------------------
class Connection
{
public:
  std::multimap<std::uint32_t, Pipe *> pipes;
  std::unordered_map<std::uint32_t, std::uint32_t> sockPipes;
  void addPipe(Pipe* p);
  void addSockPipe(std::uint32_t pi, std::uint32_t sz);
  Pipe *getPipe(std::uint32_t pi) {
    auto it = pipes.find(pi);
    if (pipes.end() == it) {
      BOOST_THROW_EXCEPTION(std::runtime_error("Invalid pipeId in getRecvPtr:" + std::to_string(pi)));
    }
    return it->second;
  }
  std::uint32_t getTranSz(std::uint32_t pipeId)
  {
    auto it = pipes.find(pipeId);
    if (pipes.end() == it)
      return 0;
    return it->second->tranSz;
  }
};

extern std::unique_ptr<Connection> connection;

class SocketClient
{
public:
  SocketClient(boost::asio::io_service& io_service,
      tcp::resolver::iterator endpoint_iterator)
    : io_service_(io_service),
      socket_(io_service)
  {
    do_connect(endpoint_iterator);
  }

  void write(std::shared_ptr<TransBase> p)
  {
    std::cout << "SocketClient::write id:" << std::hex << *(p->pipeId) << std::dec << " bytes:" << p->tranSz << "\n";
    io_service_.post(
        [this, p]()
        {
          bool write_in_progress = !write_msgs_.empty();
          write_msgs_.emplace(p);
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
        [this](boost::system::error_code ec, std::size_t length)
        {
          auto tranSz = connection->getTranSz(pipeId);
          std::cout << "SocketClient::do_read_header read:" << length << " bytes & tranSz:" << tranSz << "\n";
          if ((0 == ec) and (0 != tranSz)) {
            do_read_body(connection->getPipe(pipeId));
          } else if (ec == boost::asio::error::eof) {
          } else {
            socket_.close();
          }
        });
  }

  void do_read_body(Pipe *p)
  {
    boost::asio::async_read(socket_,
       boost::asio::buffer(p->_recvdata.get(), p->tranSz),
        [this, p](boost::system::error_code ec, std::size_t length)
        {
          std::cout << "SocketClient::do_read_body bytes:" << length << "\n";
          if (0 == ec) {
            auto range=connection->pipes.equal_range(p->pipeId);
            for (auto it=range.first; it!=range.second; ++it) {
              it->second->receive(p->_recvdata.get());
            }
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
            write_msgs_.pop();
            if (!write_msgs_.empty())
            {
              do_write();
            }
          }
          else
          {
            std::cerr << "SocketClient : Failed during write (" << ec << ")\n";
          }
        });
  }

private:
  std::uint32_t pipeId;
  boost::asio::io_service& io_service_;
  tcp::socket socket_;
  std::queue<std::shared_ptr<TransBase>> write_msgs_;
};
extern std::unique_ptr<SocketClient> sc;
extern boost::asio::io_service io_service;
extern "C" void ClientInit();
