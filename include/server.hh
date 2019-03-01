#pragma once

#define EMUC_SERVER_HH

#ifdef EMUC_CLIENT_HH
#error "Include either server.hh or client.hh!"
#endif

extern boost::asio::io_service io_service;
//----------------------------------------------------------------------
class SocketServer : public std::enable_shared_from_this<SocketServer>
{
public:
  static std::queue<std::shared_ptr<TransBase>> write_msgs_pend;
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
    while (write_msgs_pend.size() > 0) {
      write_msgs_.emplace(write_msgs_pend.front());
      write_msgs_pend.pop();
    }
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
        boost::asio::buffer((void *)&header, sizeof(header)),
        [this, self](boost::system::error_code ec, std::size_t length)
        {
          std::cout << "SocketServer::do_read_header() pipeId obtained was :" << std::hex << header.pipeId << std::dec << " ec:" << ec << "\n";
          if (0 == ec) {
	    auto p = connection.getPipe(header);
            if (0 != header.sizeOf) do_read_body(p);
          } else if (ec == boost::asio::error::eof) {
          } else {
            std::cerr << "Error while Read Header ec(" << ec << "), sizeof(pipeId)(" << sizeof(header.pipeId) << ") size(" << header.sizeOf << ") length(" << length << ")\n";
          }
        });
  }

  void do_read_body(Pipe *p)
  {
    auto self(shared_from_this());
    boost::asio::async_read
      (socket_,
       boost::asio::buffer(p->_recvdata.get(), p->tranSz),
       [this, self, p](boost::system::error_code ec, std::size_t length)
       {
	 std::cout << "SocketServer::do_read_body() ec:" << ec << " length:" << length << "\n";
         if (0 == ec) {
           auto range=connection.pipes.equal_range(p->pipeId);
           for (auto it=range.first; it!=range.second; ++it) {
             it->second->receive(p->_recvdata.get()+sizeof(p->pipeId));
           }
           do_read_header();
         } else if (ec == boost::asio::error::eof) {
         } else {
           std::cerr << "Error while Read Body ec(" << ec << "), pipeId(" << p->pipeId << ")\n";
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
            std::cerr << "Error while Write Body ec(" << ec << "), pipeId(" << write_msgs_.front()->header->pipeId << ")\n";
          }
        });
  }

  TransHeader header;
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


extern boost::asio::io_service io_service;
extern "C" void ServerInit();
