#include <deque>
#include <iostream>
#include <unordered_map>
#include <boost/asio.hpp>
#include "server.hh"

#include <cstring>

template<std::uint32_t id, std::uint32_t sz>
class transConsumer : public Consumer
{
public:
  std::unique_ptr<char[]> _data;
  transConsumer() :
    Consumer(id, sz), _data{std::make_unique<char[]>(sz+sizeof(pipeId))} {
  }
  char* getWrPtr() { return _data.get(); }
  void receive(char *d)
  {
    std::cout << "Server Obtained: " << std::string{d, 16} << "\n";
  }
};

boost::asio::io_service io_service;
tcp::endpoint ep{tcp::v4(), 9001};
std::shared_ptr<Server> serverp;
transConsumer<0, 16> t;

extern "C"
void initFunc()
{
  serverp = std::make_shared<Server>(io_service, ep);
  connection.addConsumer(&t);
  std::memset(t._data.get(), 0, 20);
  strncpy(t._data.get()+4, "Hello World....\n", 16);
  connection.write(&t);
}

extern "C"
void pollOnce()
{
  io_service.poll();
}
