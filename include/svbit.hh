#include "svdpi.h"

/* How many 32-bit locations required to store this ? */
constexpr std::size_t SV_SIZE(std::size_t size)
{ return SV_CANONICAL_SIZE(size); }

template<std::uint32_t S>
struct svBitTp {
public:
  svBitVec32* const Value;
  svBitTp() = delete;
  svBitTp(svBitVec32 *v) : Value(v) {}

  /* Type conversions */
  operator uint64_t();
  operator std::string();

  svBitTp<S>& operator=(const svBitTp<S> &o) {
    std::memcpy(Value, o.Value, SV_SIZE(S)*sizeof(std::uint32_t));
    return *this;
  }
  svBitTp<S>& operator=(const std::string& rhs);
  svBitTp<S>& operator=(const std::uint64_t rhs);
};

template<std::uint32_t S>
svBitTp<S>::operator std::string()
{
  static const char int2hex[] {"0123456789ABCDEF"};
  std::string ret;
  for (std::uint32_t ui1=SV_SIZE(S); ui1; ) {
    ui1--;
    ret+=int2hex[(Value[ui1]>>28) & 0xF];
    ret+=int2hex[(Value[ui1]>>24) & 0xF];
    ret+=int2hex[(Value[ui1]>>20) & 0xF];
    ret+=int2hex[(Value[ui1]>>16) & 0xF];
    ret+=int2hex[(Value[ui1]>>12) & 0xF];
    ret+=int2hex[(Value[ui1]>>8) & 0xF];
    ret+=int2hex[(Value[ui1]>>4) & 0xF];
    ret+=int2hex[Value[ui1] & 0xF];
  }
  //std::cout << "std::string() : " << ret << "\n";
  return ret;
}

template<std::uint32_t S>
svBitTp<S>& svBitTp<S>::operator=(const std::string& rhs)
{
  std::uint32_t idx = 0;
  for (auto c=rhs.rbegin(); c!=rhs.rend(); ) {
    std::uint32_t _idx = 0, _value;
    auto v = *c;

    for (uint32_t _idx=0; _idx<(sizeof(std::uint32_t)*2); _idx++, c++) {
      if (c == rhs.rend()) break;
      v = *c;
      _value = ((v >= 'a') && (v <= 'f')) ?
        (v-'a'+10) :
        ((v >= 'A') && (v <= 'F')) ? (v-'A'+10) :
        (v-'0');
      _value <<= (_idx*4);
      Value[idx] |= _value;
    }

    /* */
    //std::cout << std::hex << Value[idx] << std::dec << "\n";
    idx++;
    if (idx >= SV_SIZE(S)) break;
  }

  return *this;
}

template<std::uint32_t S>
svBitTp<S>::operator uint64_t()
{
  uint64_t ret = 0;
  if (SV_SIZE(S) > 1) {
    ret = Value[1];
    ret <<= (sizeof(svBitVec32)<<3);
  }
  ret |= Value[0];
  return ret;
}

template<std::uint32_t S>
svBitTp<S>& svBitTp<S>::operator=(const std::uint64_t rhs)
{
  Value[0] = rhs;
  if (SV_SIZE(S) > 1) {
    Value[1] = rhs >> 32;
//    if (SV_SIZE(S) > 2) {
//      Value[2] = rhs >> (32*2);
//      if (SV_SIZE(S) > 3) {
//        Value[3] = rhs >> (32*3);
//      }
//    }
  }

  return *this;
}
