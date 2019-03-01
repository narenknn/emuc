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
  svBitTp<64> addr{_dptr};
  svBitTp<64> data{_dptr+SV_SIZE(64)};
  static constexpr std::size_t
  DATA_U32_SZ() { return SV_SIZE(64)+SV_SIZE(64); }
  static constexpr std::size_t
  DATA_U8_SZ() { return DATA_U32_SZ()*sizeof(std::uint32_t); }
  SdpReqBus(): TransBase{DATA_U8_SZ()},
    _dptr{(std::uint32_t*)(_data.get()+sizeof(TransHeader))} {
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
