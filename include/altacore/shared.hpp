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
      Modulo,
      LeftShift,
      RightShift,
      BitwiseAnd,
      BitwiseOr,
      BitwiseXor,
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
      Plus,
      Minus,
      PreIncrement,
      PostIncrement,
      PreDecrement,
      PostDecrement,
      BitwiseNot,
    };

    enum class AssignmentType {
      Simple,
      Addition,
      Subtraction,
      Multiplication,
      Division,
      Modulo,
      LeftShift,
      RightShift,
      BitwiseAnd,
      BitwiseOr,
      BitwiseXor,
    };

    enum class ClassOperatorType {
      NONE,
      Not,
      Dereference,
      Reference,
      Addition,
      Subtraction,
      Multiplication,
      Division,
      Modulo,
      Xor,
      LeftShift,
      RightShift,
      BitAnd,
      BitOr,
      BitNot,
      And,
      Or,
      Equality,
      Inequality,
      LessThan,
      GreaterThan,
      LessThanOrEqualTo,
      GreaterThanOrEqualTo,
      Index,
      AdditionAssignment,
      SubtractionAssignment,
      MultiplicationAssignment,
      DivisionAssignment,
      ModuloAssignment,
      LeftShiftAssignment,
      RightShiftAssignment,
      BitwiseAndAssignment,
      BitwiseOrAssignment,
      BitwiseXorAssignment,
    };

    static const char* const ClassOperatorType_names[] = {
      "NONE",
      "Not",
      "Dereference",
      "Reference",
      "Addition",
      "Subtraction",
      "Multiplication",
      "Division",
      "Modulo",
      "Xor",
      "LeftShift",
      "RightShift",
      "BitAnd",
      "BitOr",
      "BitNot",
      "And",
      "Or",
      "Equality",
      "Inequality",
      "LessThan",
      "GreaterThan",
      "LessThanOrEqualTo",
      "GreaterThanOrEqualTo",
      "Index",
      "AdditionAssignment",
      "SubtractionAssignment",
      "MultiplicationAssignment",
      "DivisionAssignment",
      "ModuloAssignment",
      "LeftShiftAssignment",
      "RightShiftAssignment",
      "BitwiseAndAssignment",
      "BitwiseOrAssignment",
      "BitwiseXorAssignment",
    };

    enum class ClassOperatorOrientation {
      // indicates the operator takes a single argument
      Unary,
      // indicates the operator's `this` argument is on the left of the symbol
      Left,
      // indicates the operator's `this` argument is on the right of the symbol
      Right,
    };

    static const char* const ClassOperatorOrientation_names[] = {
      "Unary",
      "Left",
      "Right",
    };

    // RTC = regular-to-class
    static inline ClassOperatorType convertOperatorTypeRTC(OperatorType op) {
      ClassOperatorType result = ClassOperatorType::NONE;
      #define AC_OP_CONV(source, dest) if (op == OperatorType::source) {\
                                         return ClassOperatorType::dest;\
                                       }
      #define AC_OP_DCONV(name) AC_OP_CONV(name, name)

      AC_OP_DCONV(Addition);
      AC_OP_DCONV(Subtraction);
      AC_OP_DCONV(Multiplication);
      AC_OP_DCONV(Division);
      AC_OP_DCONV(Modulo);

      AC_OP_DCONV(LeftShift);
      AC_OP_DCONV(RightShift);

      AC_OP_CONV(BitwiseAnd, BitAnd);
      AC_OP_CONV(BitwiseOr, BitOr);
      AC_OP_CONV(BitwiseXor, Xor);

      AC_OP_CONV(EqualTo, Equality);
      AC_OP_CONV(NotEqualTo, Inequality);

      AC_OP_DCONV(LessThan);
      AC_OP_DCONV(LessThanOrEqualTo);
      AC_OP_DCONV(GreaterThan);
      AC_OP_DCONV(GreaterThanOrEqualTo);

      AC_OP_CONV(LogicalAnd, And);
      AC_OP_CONV(LogicalOr, Or);

      #undef AC_OP_CONV
      #undef AC_OP_DCONV
      return ClassOperatorType::NONE;
    };
    // RTC = regular-to-class
    static inline ClassOperatorType convertOperatorTypeRTC(UOperatorType op) {
      ClassOperatorType result = ClassOperatorType::NONE;
      #define AC_OP_CONV(source, dest) if (op == UOperatorType::source) {\
                                         return ClassOperatorType::dest;\
                                       }
      #define AC_OP_DCONV(name) AC_OP_CONV(name, name)

      AC_OP_DCONV(Not);
      AC_OP_CONV(BitwiseNot, BitNot);

      #undef AC_OP_CONV
      #undef AC_OP_DCONV
      return ClassOperatorType::NONE;
    };
    // RTC = regular-to-class
    static inline ClassOperatorType convertOperatorTypeRTC(AssignmentType op) {
      ClassOperatorType result = ClassOperatorType::NONE;
      #define AC_OP_CONV(source, dest) if (op == AssignmentType::source) {\
                                         return ClassOperatorType::dest;\
                                       }
      #define AC_OP_ACONV(name) AC_OP_CONV(name, name##Assignment)

      AC_OP_ACONV(Addition);
      AC_OP_ACONV(Subtraction);
      AC_OP_ACONV(Multiplication);
      AC_OP_ACONV(Division);
      AC_OP_ACONV(Modulo);
      AC_OP_ACONV(LeftShift);
      AC_OP_ACONV(RightShift);
      AC_OP_ACONV(BitwiseAnd);
      AC_OP_ACONV(BitwiseOr);
      AC_OP_ACONV(BitwiseXor);

      #undef AC_OP_CONV
      #undef AC_OP_ACONV
      return ClassOperatorType::NONE;
    };
  };
};

#endif // ALTACORE_SHARED_HPP
