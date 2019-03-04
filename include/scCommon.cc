std::queue<std::shared_ptr<TransBase>> _write_msgs_pend;
std::multimap<std::uint32_t, Pipe *> _pipes;
std::map<std::uint32_t, std::unique_ptr<UnusedPipe>> _unused_pipes;
Connection connection;
PipeConnector pipe0;

void
Pipe::rawSend(std::shared_ptr<TransBase>& p)
{
  /* local send */
  auto range=_pipes.equal_range(pipeId);
  for (auto it=range.first; it!=range.second; ++it) {
    if (this != it->second)
      it->second->receive(p->_data.get());
  }
  /* socket send */
  if (ss) {
    ss->write(p);
  } else {
    _write_msgs_pend.emplace(p);
  }
}

void
Pipe::send(std::shared_ptr<TransBase> p)
{
  if (not isConnected) {
    std::cerr << "Attempt to send in Pipe (" << std::hex << pipeId << std::dec << ") that is not yet connected..\n";
    return;
  }
  //std::cout << "Sending pipeId:" << std::hex << pipeId << std::dec << " sizeOf:" << p->tranSz << "\n";
  p->header->pipeId = pipeId;
  p->header->sizeOf = p->tranSz;
  rawSend(p);
}

Pipe::Pipe(std::uint32_t p, std::uint32_t tz) :
  pipeId(p), tranSz(tz), _recvdata{std::make_unique<char[]>(tz)} {
}

void
Pipe::connect()
{
  isConnected = true;
  Connection::addPipe(this);
}

/* Local connection sits in 'pipes' */
void
Connection::addPipe(Pipe *p)
{
  if (_pipes.end() == _pipes.find(p->pipeId)) {
    //std::cout << "addPipe adding pipeId:" << std::hex << p->pipeId << std::dec << " to pipes\n";
    _pipes.emplace(p->pipeId, p);
  }
  //for (auto it: _pipes) { std::cout << "Connection::addPipe():_pipes got pipeId:" << std::hex << it.first << std::dec << "\n"; }
  //std::cout << "addPipe called for (" << p << ") pipeId:" << std::hex << p->pipeId << std::dec << "\n";
  /* tell to other end of this pipe */
  auto trans = std::make_shared<TransBase>(sizeof(std::uint32_t)+sizeof(std::uint32_t));
  trans->header->pipeId = CRC32_STR("//connect//");
  trans->header->sizeOf = sizeof(std::uint32_t)+sizeof(std::uint32_t);
  std::memcpy(trans->getWrPtr()+sizeof(trans->header), &(p->pipeId), sizeof(std::uint32_t));
  std::memcpy(trans->getWrPtr()+sizeof(trans->header)+sizeof(std::uint32_t), &(p->tranSz), sizeof(std::uint32_t));
  p->rawSend(trans);
}

PipeConnector::PipeConnector():
  Pipe{CRC32_STR("//connect//"),
    sizeof(std::uint32_t)+sizeof(std::uint32_t)} {
};

void
PipeConnector::receive(char *d)
{
  /* add socket pipe */
  //std::cout << "PipeConnector::receive called pi:" << std::hex << pipeId << std::dec << " sz:" << tranSz << "\n";
}
