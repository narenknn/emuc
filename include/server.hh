#pragma once

#define EMUC_SERVER_HH

#ifdef EMUC_CLIENT_HH
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
  const std::uint32_t pipeId, tranSz;
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

  void write(std::shared_ptr<TransBase> p)
  {
    bool write_in_progress = !write_msgs_.empty();
    write_msgs_.emplace(p);
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
          auto tranSz = connection.getTranSz(pipeId);
          if ((0 == ec) and (0 != tranSz)) {
              do_read_body(connection.getPipe(pipeId));
          } else if (ec == boost::asio::error::eof) {
          } else {
            std::cerr << "Error while Read Header ec(" << ec << "), sizeof(pipeId)(" << sizeof(pipeId) << ") size(" << tranSz << ") length(" << length << ")\n";
          }
        });
  }

  void do_read_body(Pipe *p)
  {
    auto self(shared_from_this());
    boost::asio::async_read
      (socket_,
       boost::asio::buffer(p->_recvdata.get(), p->tranSz),
       [this, self, p](boost::system::error_code ec, std::size_t /*length*/)
       {
         if (0 == ec) {
           auto range=connection.pipes.equal_range(p->pipeId);
           for (auto it=range.first; it!=range.second; ++it) {
             it->second->receive(p->_recvdata.get());
           }
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
            write_msgs_.pop();
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
  std::queue<std::shared_ptr<TransBase>> write_msgs_;
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
