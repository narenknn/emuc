#include <queue>
#include <iostream>
#include <unordered_map>
#include <boost/asio.hpp>
#include <thread>
#include "scCommon.hh"
#include "client.hh"

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
    _dptr{(std::uint32_t*)(_data.get()+sizeof(TransHeader))} {
  }
  SdpReqBus(char *d): SdpReqBus() {
    std::memcpy(_dptr, d, DATA_U8_SZ());
  }
  SdpReqBus& operator=(const SdpReqBus& o) { /* copy assignment */
    std::memcpy(_dptr, o._dptr, DATA_U8_SZ());
  }
  SdpReqBus(const SdpReqBus& o):
    TransBase(o),
    _dptr{(std::uint32_t*)(_data.get()+sizeof(TransHeader))} { /* copy constructor */
  }
  SdpReqBus(SdpReqBus&& o) noexcept : /* move constructor */
    TransBase(o), _dptr{(std::uint32_t*)(_data.get()+sizeof(TransHeader))} {
  }
};

//----------------------------------------------------------------------
template<std::uint32_t PipeId, class Bus>
class EmuTransactor : public Pipe
{
public:
  EmuTransactor() : Pipe{PipeId, Bus::DATA_U8_SZ()}
  { }
  void receive(char *d)
  {
    Bus b{d};
    std::cout << "EmuTransactor Obtained transaction addr:" << std::string(b.addr) << " data:" << std::string(b.data) << "\n";
  }
};
EmuTransactor<CRC32_STR("abcd"), SdpReqBus> trans;

int main(int argc, char* argv[])
{
  try {
    pollInit();
    trans.connect();
    std::shared_ptr<SdpReqBus> v1{std::make_shared<SdpReqBus>()};
    v1->addr = 0xABCD;
    v1->data = 0xABCC;
    std::cout << "Sending addr:" << std::string(v1->addr) << " data:" << std::string(v1->data) << "\n";
    trans.send(v1);
    //std::cout << "v1.. pipeId:" << std::hex << v1->header->pipeId << std::dec << " sizeOf:" << v1->header->sizeOf << "\n";

    while (true) {
      //    for (auto i=0; i<1000; i++) {
      sleep(0.01);
      pollOnce();
    }
    ss->close();
  } catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << "\n";
  } catch (...) {
    std::cerr << "Exception ocurred!!\n";
  }

  return 0;
}
