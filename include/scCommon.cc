std::queue<std::shared_ptr<TransBase>> SS::write_msgs_pend;
std::multimap<std::uint32_t, Pipe *> Connection::pipes;
std::map<std::uint32_t, std::unique_ptr<UnusedPipe>> Connection::unused_pipes;
Connection connection;
PipeConnector pipe0;

void
Pipe::rawSend(std::shared_ptr<TransBase>& p)
{
  /* local send */
  auto range=connection.pipes.equal_range(pipeId);
  for (auto it=range.first; it!=range.second; ++it) {
    if (this != it->second)
      it->second->receive(p->_data.get());
  }
  /* socket send */
  if (ss) {
    ss->write(p);
  } else {
    SS::write_msgs_pend.emplace(p);
  }
}

Pipe::Pipe(std::uint32_t p, std::uint32_t tz) :
  pipeId(p), tranSz(tz), _recvdata{std::make_unique<char[]>(tz)} {
  Connection::addPipe(this);
}

/* Local connection sits in 'pipes' */
void
Connection::addPipe(Pipe *p)
{
  if (pipes.end() == pipes.find(p->pipeId)) {
    pipes.emplace(p->pipeId, p);
  }
  /* tell to other end of this pipe */
  auto trans = std::make_shared<TransBase>(sizeof(std::uint32_t)+sizeof(std::uint32_t));
  trans->header->pipeId = CRC32_STR("//connect//");
  std::memcpy(trans->getWrPtr()+sizeof(trans->header), &(p->pipeId), sizeof(std::uint32_t));
  std::memcpy(trans->getWrPtr()+sizeof(trans->header)+sizeof(std::uint32_t), &(p->tranSz), sizeof(std::uint32_t));
  if (ss) ss->write(trans);
  else SS::write_msgs_pend.emplace(trans);
}

PipeConnector::PipeConnector():
  Pipe{CRC32_STR("//connect//"),
    sizeof(std::uint32_t)+sizeof(std::uint32_t)} {
};

void
PipeConnector::receive(char *d)
{
  std::uint32_t pi, sz;
  std::memcpy(&pi, d, sizeof(pi));
  std::memcpy(&sz, d+sizeof(pi), sizeof(sz));
  /* add socket pipe */
  std::cout << "PipeConnector::receive called pi:" << std::hex << pi << std::dec << " sz:" << sz << "\n";
}
