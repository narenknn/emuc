#include <queue>
#include <iostream>
#include <unordered_map>
#include <boost/asio.hpp>
#include "server.hh"

#include <cstring>

#include "svbit.hh"
#include "shash.hh"

class SdpReqBus : public TransBase {
public:
  std::uint32_t* const _dptr;
  svBitTp<64> addr{_dptr};
  svBitTp<64> data{_dptr+SV_SIZE(64)};
  static constexpr std::size_t
  DATA_U32_SZ() { return SV_SIZE(64)+SV_SIZE(64); }
  static constexpr std::size_t
  DATA_U8_SZ() { return DATA_U32_SZ()*sizeof(std::uint32_t); }
  SdpReqBus(): TransBase{DATA_U8_SZ()},
    _dptr{(std::uint32_t*)(((char *)_data.get())+sizeof(*pipeId))} {
  }
  SdpReqBus(char *_d): TransBase{DATA_U8_SZ()},
    _dptr{(std::uint32_t*)(((char *)_data.get())+sizeof(*pipeId))} {
      std::memcpy(_dptr, _d, DATA_U8_SZ());
  }
  SdpReqBus& operator=(SdpReqBus& o) {
    std::memcpy(_dptr, o._dptr, DATA_U8_SZ());
  }
};

template<std::uint32_t PipeId, class Bus>
class EmuTransactor : public Pipe
{
public:
  EmuTransactor() : Pipe{PipeId, Bus::DATA_U8_SZ()}
  { }
  void send(std::shared_ptr<TransBase> p)
  {
    *(p->pipeId) = PipeId;
    rawSend(p);
  }
  void receive(char *)
  {
    //Bus{_recvdata.get()};
    std::cout << "Server Obtained transaction\n";
  }
};

EmuTransactor<CRC32_STR("abcd"), SdpReqBus> trans;

int main(int argc, char* argv[])
{
  std::cout << "PipeId of trans: " << std::hex << CRC32_STR("abcd") << std::dec << "\n";
  std::shared_ptr<SdpReqBus> v0{std::make_shared<SdpReqBus>()};
  ServerInit();
//  trans = std::make_unique<EmuTransactor<CRC32_STR("abcd"), SdpReqBus>>();
//  trans->send(v0);
  io_service.run();

  return 0;
}
