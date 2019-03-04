#pragma once

#define EMUC_SERVER_HH

#ifdef EMUC_CLIENT_HH
#error "Include either server.hh or client.hh!"
#endif

extern std::queue<std::shared_ptr<TransBase>> _write_msgs_pend;
extern boost::asio::io_service io_service;

//----------------------------------------------------------------------
class SocketServer : public std::enable_shared_from_this<SocketServer>
{
public:
  bool write_in_progress{false}, isConnected{false};
  SocketServer(tcp::socket socket)
    : socket_(std::move(socket))
  {
  }

  void start()
  {
    isConnected = true;
    do_read_header();
  }

  void write(std::shared_ptr<TransBase> p)
  {
    while (_write_msgs_pend.size() > 0) {
      write_msgs_.emplace(_write_msgs_pend.front());
      _write_msgs_pend.pop();
    }
    write_msgs_.emplace(p);
    if (isConnected and not write_in_progress)
    {
      do_write();
    }
  }

  void serviceLoop()
  {
    while (_write_msgs_pend.size() > 0) {
      write_msgs_.emplace(_write_msgs_pend.front());
      _write_msgs_pend.pop();
    }
    //std::cout << "serviceLoop isConnected:" << (isConnected?"true":"false") << " write_in_progress:" << (write_in_progress?"true":"false") << "\n";
    if (isConnected and not write_in_progress and not write_msgs_.empty()) {
      do_write();
    }
    //for (auto it: _pipes) { std::cout << "serviceLoop():_pipes got pipeId:" << std::hex << it.first << std::dec << "\n"; }
  }

private:
  void do_read_header()
  {
    auto self(shared_from_this());
    boost::asio::async_read(socket_,
        boost::asio::buffer((void *)&header, sizeof(header)),
        [this, self](boost::system::error_code ec, std::size_t length)
        {
          //std::cout << "SocketServer::do_read_header() pipeId obtained was :" << std::hex << header.pipeId << std::dec << " ec:" << ec << " length:" << length << " header.sizeof:" << header.sizeOf << "\n";
          if (0 == ec) {
	    if (length == sizeof(header)) {
	      auto p = connection.getPipe(header);
	      if (0 != header.sizeOf) do_read_body(p);
	    }
          } else if (ec == boost::asio::error::eof) {
          } else {
            std::cerr << "Error while Read Header ec(" << ec << "), sizeof(pipeId)(" << sizeof(header.pipeId) << ") size(" << header.sizeOf << ") length(" << length << ")\n";
          }
        });
  }

  void do_read_body(Pipe *p)
  {
    auto self(shared_from_this());
    std::memcpy(p->_recvdata.get(), &header, sizeof(header));
    boost::asio::async_read
		(socket_, boost::asio::buffer(p->_recvdata.get()+sizeof(header), p->tranSz),
       [this, self, p](boost::system::error_code ec, std::size_t length)
       {
	 //std::cout << "SocketServer::do_read_body() ec:" << ec << " length:" << length << "\n";
         if (0 == ec) {
           auto range=_pipes.equal_range(p->pipeId);
           for (auto it=range.first; it!=range.second; ++it) {
	     it->second->receive(p->_recvdata.get());
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
    write_in_progress = true;
    //std::cout << "SocketServer::do_write() called\n";
    boost::asio::async_write
      (socket_,
       boost::asio::buffer(write_msgs_.front()->getWrPtr(),
                           write_msgs_.front()->getWrPtrSz()),
        [this, self](boost::system::error_code ec, std::size_t length)
        {
          std::cout << "Wrote length:" << length << " pipeId:" << std::hex << write_msgs_.front()->header->pipeId << std::dec << " sizeof:" << write_msgs_.front()->header->sizeOf << "\n";
	  write_in_progress = false;
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
  Server(boost::asio::io_service& io_s,
      const tcp::endpoint& endpoint)
    : acceptor_(io_s, endpoint),
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

  tcp::socket socket_;
  tcp::acceptor acceptor_;
};

extern "C" void pollOnce();
extern "C" void pollInit();
