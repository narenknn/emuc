#pragma once

#define EMUC_CLIENT_HH

#ifdef EMUC_SERVER_HH
#error "Include either server.hh or client.hh!"
#endif

class SocketClient
{
public:
  static std::queue<std::shared_ptr<TransBase>> write_msgs_pend;
  SocketClient(boost::asio::io_service& io_service,
	       tcp::resolver::iterator endpoint_iterator):
    io_service_(io_service), socket_(io_service)
  {
    do_connect(endpoint_iterator);
  }

  void write(std::shared_ptr<TransBase> p)
  {
    std::cout << "SocketClient::write id:" << std::hex << p->header->pipeId << std::dec << " bytes:" << p->tranSz << " vs bytes-in-header:" << p->header->sizeOf << "\n";
    io_service_.post(
        [this, p]()
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
        boost::asio::buffer((void *)&header, sizeof(header)),
        [this](boost::system::error_code ec, std::size_t length)
        {
          std::cout << "SocketClient::do_read_header PipeId:" << std::hex << header.pipeId << std::dec << " read:" << length << " bytes & sizeOf:" << header.sizeOf << "\n";
          if ((0 == ec) and (0 != header.sizeOf)) {
	    auto p = connection.getPipe(header);
            if (0 != header.sizeOf) do_read_body(p);
          } else if (ec == boost::asio::error::eof) {
          } else {
            std::cerr << "Error while Read Header ec(" << ec << "), sizeof(header)(" << sizeof(header) << ") size(" << header.sizeOf << ") length(" << length << ")\n";
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
            auto range=connection.pipes.equal_range(p->pipeId);
            for (auto it=range.first; it!=range.second; ++it) {
              it->second->receive(p->_recvdata.get()+sizeof(p->pipeId));
            }
            do_read_header();
          } else if (ec == boost::asio::error::eof) {
          } else {
            std::cerr << "Error while Read Body ec(" << ec << "), p->tranSz:" << p->tranSz << ") length(" << length << ")\n";
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
            std::cerr << "Error while Write Body ec(" << ec << "), pipeId(" << write_msgs_.front()->header->pipeId << ")\n";
          }
        });
  }

private:
  TransHeader header;
  boost::asio::io_service& io_service_;
  tcp::socket socket_;
  std::queue<std::shared_ptr<TransBase>> write_msgs_;
};

extern boost::asio::io_service io_service;
extern std::unique_ptr<SocketClient> ss;
extern "C" void ClientInit();
