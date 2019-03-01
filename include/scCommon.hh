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
  virtual void send(std::shared_ptr<TransBase> p)
  {
    std::cout << "Sending pipeId:" << std::hex << pipeId << std::dec << "\n";
    p->header->pipeId = pipeId;
    p->header->sizeOf = p->tranSz;
    rawSend(p);
  }
  virtual void receive(char *) = 0;

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
class Connection
{
public:
  static std::multimap<std::uint32_t, Pipe *> pipes;
  static std::map<std::uint32_t, std::unique_ptr<UnusedPipe>> unused_pipes;
  static void addPipe(Pipe* p);
  Pipe *getPipe(const TransHeader& header) {
    auto it = pipes.find(header.pipeId);
    if (pipes.end() != it)
      return it->second;
    /* create a new unused pipe */
    if (unused_pipes.end() == unused_pipes.find(header.pipeId))
      unused_pipes.emplace(header.pipeId, std::make_unique<UnusedPipe>(header.pipeId, header.sizeOf));
    return unused_pipes[header.pipeId].get();
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
    std::cout << "Client Obtained transaction\n";
  }
};
