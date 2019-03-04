#include <queue>
#include <iostream>
#include <unordered_map>
#include <boost/asio.hpp>
#include "scCommon.hh"
#include "server.hh"

#include <cstring>

#include "svbit.hh"
#include "shash.hh"

class SdpReqBus : public TransBase {
public:
  std::uint32_t* const _dptr;
  svBitTp<64> addr;
  svBitTp<64> data;
  static constexpr std::size_t
  DATA_U32_SZ() { return SV_SIZE(64)+SV_SIZE(64); }
  static constexpr std::size_t
  DATA_U8_SZ() { return DATA_U32_SZ()*sizeof(std::uint32_t); }
  SdpReqBus(): TransBase{DATA_U8_SZ()},
	       _dptr{(std::uint32_t*)(_data.get()+sizeof(TransHeader))},
	       addr{_dptr},
	       data{_dptr+SV_SIZE(64)} {
  }
  SdpReqBus(char *d): SdpReqBus() {
    std::memcpy(_dptr, d, DATA_U8_SZ());
  }
  SdpReqBus& operator=(const SdpReqBus& o) { /* copy assignment */
    std::memcpy(_dptr, o._dptr, DATA_U8_SZ());
  }
  SdpReqBus(const SdpReqBus& o):
    SdpReqBus{} { /* copy constructor */
    std::memcpy(_data.get(), o._data.get(), getWrPtrSz());
  }
  SdpReqBus(SdpReqBus&& o) noexcept : /* move constructor */
    SdpReqBus{} {
    std::memcpy(_data.get(), o._data.get(), getWrPtrSz());
  }
};

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
  std::cout << "PipeId of EmuTransactor:abcd:" << std::hex << CRC32_STR("abcd") << std::dec << "\n";
  pollInit();
  trans.connect();
  std::shared_ptr<SdpReqBus> v0{std::make_shared<SdpReqBus>()};
  v0->addr = 0xABCD;
  v0->data = 0xABC5;
  std::cout << "Sending addr:" << std::string(v0->addr) << " data:" << std::string(v0->data) << "\n";
  trans.send(v0);

  while (true) {
    //  for (auto i=0; i<1000; i++) {
    sleep(0.01);
    pollOnce();
  }

  return 0;
}
