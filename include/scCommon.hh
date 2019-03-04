#pragma once

//----------------------------------------------------------------------
using boost::asio::ip::tcp;
class Connection;

struct TransHeader {
  std::uint32_t pipeId, sizeOf;
};

//----------------------------------------------------------------------
class TransBase
{
public:
  const std::uint32_t tranSz;
  std::unique_ptr<char[]> _data;
  TransHeader *const header;
  char *getWrPtr() { return _data.get(); }
  std::uint32_t getWrPtrSz() { return sizeof(header) + tranSz; }
  TransBase(std::uint32_t tsz) :
    tranSz(tsz), _data{std::make_unique<char[]>(tsz+sizeof(header))},
    header{(TransHeader *)_data.get()}
  { }
  TransBase(const TransBase& o) :
    TransBase(o.tranSz) {
    std::memcpy(_data.get(), o._data.get(), getWrPtrSz());
  }
  TransBase(TransBase&& o) noexcept :
    tranSz(o.tranSz), _data{std::move(o._data)},
    header{(TransHeader *)_data.get()}
  { }
};

//----------------------------------------------------------------------
class Pipe
{
public:
  const std::uint32_t pipeId, tranSz;
  std::unique_ptr<char[]> _recvdata;
  bool isConnected{false};

  void rawSend(std::shared_ptr<TransBase>& p);
  virtual void send(std::shared_ptr<TransBase> p);
  virtual void receive(char *) = 0;
  void connect();

  Pipe(std::uint32_t p, std::uint32_t tz);
};

class UnusedPipe : public Pipe
{
public:
  void receive(char *) {}
  UnusedPipe(std::uint32_t pi, std::uint32_t sz) :
    Pipe{pi, sz} { }
};

class PipeConnector : public Pipe
{
public:
  void receive(char *);
  PipeConnector();
};
extern PipeConnector pipe0;

//----------------------------------------------------------------------
extern std::multimap<std::uint32_t, Pipe *> _pipes;
extern std::map<std::uint32_t, std::unique_ptr<UnusedPipe>> _unused_pipes;
class Connection
{
public:
  static void addPipe(Pipe* p);
  Pipe *getPipe(const TransHeader& header) {
    auto it = _pipes.find(header.pipeId);
    //std::cout << "getPipe for id:" << std::hex << header.pipeId << std::dec << ((_pipes.end() == it) ? " not found" : " found") << "\n";
    //for (auto it: _pipes) { std::cout << "_pipes got pipeId:" << std::hex << it.first << std::dec << "\n"; }
    if (_pipes.end() != it) return it->second;
    /* create a new unused pipe */
    if (_unused_pipes.end() == _unused_pipes.find(header.pipeId)) {
      //std::cout << "Creating _unused_pipes for pipeId:" << std::hex << header.pipeId << std::dec << "\n";
      _unused_pipes.emplace(header.pipeId, std::make_unique<UnusedPipe>(header.pipeId, header.sizeOf));
    }
    return _unused_pipes[header.pipeId].get();
  }
};
extern Connection connection;

//----------------------------------------------------------------------
template<std::uint32_t PipeId, class Bus>
class EmuTransactor : public Pipe
{
public:
  EmuTransactor() : Pipe{PipeId, Bus::DATA_U8_SZ()}
  { }
  void receive(char *)
  {
    //Bus{_recvdata.get()};
    std::cout << "EmuTransactor Obtained transaction\n";
  }
};
