#ifndef ALTACORE_SHARED_HPP
#define ALTACORE_SHARED_HPP

namespace AltaCore {
  namespace Shared {
    enum class TypeModifierFlag: uint8_t {
      None      = 0,
      Pointer   = 1 << 0,
      Constant  = 1 << 1,
      Reference = 1 << 2,
    };
  };
};

#endif // ALTACORE_SHARED_HPP