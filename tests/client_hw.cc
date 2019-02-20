#include <deque>
#include <iostream>
#include <unordered_map>
#include <boost/asio.hpp>
#include "client.hh"
#include <thread>

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
