template<std::uint32_t S> struct svBitT;

template<std::uint32_t S>
struct svBitTp {
public:
  svBitVec32 *Value;
  svBitTp() = delete;
  svBitTp(svBitVec32 *v) : Value(v) {}

  svBitTp& operator=(const svBitT<S> &o) {
    for (uint32_t ui1=SV_CANONICAL_SIZE(S); ui1;) {
      ui1--;
      Value[ui1] = o.Value[ui1];
    }
  }
};

template<std::uint32_t S>
struct svBitT {
public:
  svBitVec32 Value[SV_CANONICAL_SIZE(S)];

  /* Type conversions */
  operator uint64_t();
  operator svBitVec32*();
  operator std::string();

  /* Assignments */
  svBitT<S>(const char *rhs);
  svBitT<S>();
};

template<std::uint32_t S>
svBitT<S>::svBitT()
{
  for (uint32_t ui1=SV_CANONICAL_SIZE(S); ui1; ui1--) {
    Value[ui1-1] = 0;
  }
}

template<std::uint32_t S>
svBitT<S>::operator svBitVec32*()
{
  return Value;
}

template<std::uint32_t S>
svBitT<S>::operator uint64_t()
{
  uint64_t ret = 0;
  if (SV_CANONICAL_SIZE(S) > 1) {
    ret = Value[1];
    ret <<= (sizeof(svBitVec32)<<3);
  }
  ret |= Value[0];
  return ret;
}

template<std::uint32_t S>
svBitT<S>::operator std::string()
{
  static const char int2hex[] {"0123456789ABCDEF"};
  std::string ret;
  for (std::uint32_t ui1=SV_CANONICAL_SIZE(S); ui1; ) {
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
svBitT<S>::svBitT(const char* rhs)
  : svBitT()
{
  std::string srhs{rhs};
  std::uint32_t idx = 0;
  for (auto c=srhs.rbegin(); c!=srhs.rend(); ) {
    std::uint32_t _idx = 0, _value;
    auto v = *c;

    for (uint32_t _idx=0; _idx<(sizeof(std::uint32_t)*2); _idx++, c++) {
      if (c == srhs.rend()) break;
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
    if (idx >= SV_CANONICAL_SIZE(S)) break;
  }
}
