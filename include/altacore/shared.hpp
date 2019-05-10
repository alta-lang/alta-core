#ifndef ALTACORE_SHARED_HPP
#define ALTACORE_SHARED_HPP

#include <cinttypes>

namespace AltaCore {
  namespace Shared {
    enum class TypeModifierFlag: uint8_t {
      None      = 0,
      Pointer   = 1 << 0,
      Constant  = 1 << 1,
      Reference = 1 << 2,

      // native-only modifiers
      Signed    = 1 << 3,
      Unsigned  = 1 << 4,
      Long      = 1 << 5,
      Short     = 1 << 6,
    };

    enum class Visibility {
      Public,
      Protected,
      Private,
    };

    enum class OperatorType {
      Addition,
      Subtraction,
      Multiplication,
      Division,
      EqualTo,
      NotEqualTo,
      GreaterThan,
      LessThan,
      GreaterThanOrEqualTo,
      LessThanOrEqualTo,
      LogicalAnd,
      LogicalOr,
    };

    enum class UOperatorType {
      Not,
    };
  };
};

#endif // ALTACORE_SHARED_HPP
