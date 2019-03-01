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
  SdpReqBus(char *_d): TransBase{DATA_U8_SZ()},
    _dptr{(std::uint32_t*)(_data.get()+sizeof(TransHeader))} {
      std::memcpy(_dptr, _d, DATA_U8_SZ());
  }
  SdpReqBus& operator=(SdpReqBus& o) {
    std::memcpy(_dptr, o._dptr, DATA_U8_SZ());
  }
};

EmuTransactor<CRC32_STR("abcd"), SdpReqBus> trans;

int main(int argc, char* argv[])
{
  try {
    ClientInit();
    std::shared_ptr<SdpReqBus> v1{std::make_shared<SdpReqBus>()};

    trans.send(v1);

    sleep(1);

    io_service.run();

    ss->close();
  } catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << "\n";
  } catch (...) {
    std::cerr << "Exception ocurred!!\n";
  }

  return 0;
}
