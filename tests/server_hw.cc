#include <deque>
#include <iostream>
#include <unordered_map>
#include <boost/asio.hpp>
#include "server.hh"

#include <list>

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
