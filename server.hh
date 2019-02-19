#pragma once

//----------------------------------------------------------------------
class Connection;

//----------------------------------------------------------------------
class Consumer
{
public:
  std::uint32_t pipeId, tranSz;
  virtual char* getWrPtr() = 0;
  virtual std::uint32_t getWrPtrSz() { return sizeof(pipeId) + tranSz; }
  virtual void receive(char *d) = 0;
  Consumer(std::uint32_t p, std::uint32_t tsz) :
    pipeId(p), tranSz(tsz)
  {}
};

//----------------------------------------------------------------------
class Pipe
{
public:
  std::uint32_t pipeId, tranSz;
  std::unique_ptr<char[]> _data;
  std::vector<Consumer *> _consumers;
  void addConsumer(Consumer* c)
  {
    _consumers.emplace_back(c);
  }

  void receive()
  {
    std::cout << "in Pipe::receive()\n";
    for (auto& consumer: _consumers)
      consumer->receive(_data.get());
  }

  Pipe(std::uint32_t p, std::uint32_t tz) :
    pipeId(p), tranSz(tz), _data{new char[tz]} {
  }
};

class Connection
{
public:
  std::unordered_map<std::uint32_t, std::unique_ptr<Pipe>> pipes;
  void addConsumer(Consumer* c)
  {
    if (pipes.end() == pipes.find(c->pipeId))
      pipes.emplace(c->pipeId, std::make_unique<Pipe>(c->pipeId, c->tranSz));
    pipes[c->pipeId]->addConsumer(c);
  }
  std::uint32_t getTranSz(std::uint32_t pipeId)
  {
    if (pipes.end() == pipes.find(pipeId))
      return 0;
    return pipes[pipeId]->tranSz;
  }
  void write(Consumer *c);
};

extern Connection connection;
