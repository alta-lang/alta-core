#ifndef ALTACORE_DET_TYPE_HPP
#define ALTACORE_DET_TYPE_HPP

#include "scope-item.hpp"
#include "class.hpp"
#include <string>
#include <vector>

namespace AltaCore {
  namespace AST {
    class ExpressionNode;
  };
  namespace DET {
    using Shared::TypeModifierFlag;

    enum class CastComponentType {
      From,
      To,
      Upcast,
      Downcast,
      SimpleCoercion,
      Narrow,
      Widen,
      Wrap,
      Unwrap,
      Destination,
      Reference,
      Dereference,
      Multicast,
    };

    enum class SpecialCastType {
      None,
      OptionalPresent,
      EmptyOptional,
      WrapFunction,
      TestPointer,
    };

    class Type;
    class CastComponent;

    using CastPath = std::vector<CastComponent>;
    using CastCompatibility = size_t;
    constexpr CastCompatibility MAX_CAST_COMPAT = SIZE_MAX;

    class CastComponent {
      public:
        CastComponentType type = CastComponentType::Destination;
        SpecialCastType special = SpecialCastType::None;

        // CCT::From = Indicates "from" method called with previous component (current "from" previous)
        // CCT::To = Indicates "to" method called with previous component (previous "to" current)
        std::shared_ptr<Function> method = nullptr;

        // CCT::SimpleCoercion = Indicates plain cast target type
        // CCT::Narrow = Indicates type to narrow previous component to
        // CCT::Widen = Indicates type to widen previous component to
        std::shared_ptr<Type> target = nullptr;

        // CCT::Widen = Indicates the union member to use to widen
        std::shared_ptr<Type> via = nullptr;

        // CCT::Upcast = Indicates upcast target class
        // CCT::Downcast = Indicates downcast target class
        std::shared_ptr<Class> klass = nullptr;

        std::vector<std::pair<size_t, CastPath>> multicasts;

        CastComponent(CastComponentType _type):
          type(_type)
          {};
        CastComponent(CastComponentType _type, std::shared_ptr<Function> _method):
          type(_type),
          method(_method)
          {};
        CastComponent(CastComponentType _type, std::shared_ptr<Type> _target, std::shared_ptr<Type> _via = nullptr):
          type(_type),
          target(_target),
          via(_via)
          {};
        CastComponent(CastComponentType _type, std::shared_ptr<Class> _klass):
          type(_type),
          klass(_klass)
          {};
        CastComponent(CastComponentType _type, std::vector<std::pair<size_t, CastPath>> _multicasts):
          type(_type),
          multicasts(_multicasts)
          {};
        CastComponent(CastComponentType _type, SpecialCastType _special):
          type(_type),
          special(_special)
          {};

        bool operator ==(const CastComponent& other) const;
    };

    class Type: public ScopeItem {
      private:
        bool commonCompatiblity(const Type& other, bool strict = false) const;
      public:
        virtual const NodeType nodeType();
        virtual std::shared_ptr<Node> clone();
        virtual std::shared_ptr<Node> deepClone();

        std::shared_ptr<Type> copy() const;

        // why is this a static method and not a constructor?
        // because it can return `nullptr` if it doesn't find an
        // underlying type for the given expression, whereas with a
        // constructor, we can't do that
        static std::shared_ptr<Type> getUnderlyingType(DH::ExpressionNode* expression);
        static std::shared_ptr<Type> getUnderlyingType(std::shared_ptr<ScopeItem> item);
        static std::vector<std::shared_ptr<Type>> getUnderlyingTypes(DH::ExpressionNode* expression);

        static std::vector<CastPath> findAllPossibleCasts(std::shared_ptr<Type> from, std::shared_ptr<Type> to, bool manual = false);
        static CastCompatibility determineCompatiblity(std::shared_ptr<Type> from, std::shared_ptr<Type> to, CastPath cast);
        static std::pair<CastPath, CastCompatibility> findMostCompatibleCast(std::shared_ptr<Type> from, std::shared_ptr<Type> to, std::vector<CastPath> casts);
        static CastPath findCast(std::shared_ptr<Type> from, std::shared_ptr<Type> to, bool manual = false);

        bool isAny = false;
        bool isNative = true;
        bool isFunction = false;
        bool isMethod = false;
        bool isAccessor = false;
        bool throws = false;
        bool isRawFunction = true;
        std::shared_ptr<Class> methodParent = nullptr;
        NativeType nativeTypeName = NativeType::Void;
        std::string userDefinedName;
        std::shared_ptr<Class> klass = nullptr;
        std::shared_ptr<Type> returnType = nullptr;
        std::vector<std::tuple<std::string, std::shared_ptr<Type>, bool, std::string>> parameters;
        std::vector<std::shared_ptr<Type>> unionOf;
        std::shared_ptr<Class> bitfield = nullptr;

        bool isOptional = false;
        std::shared_ptr<Type> optionalTarget = nullptr;

        const size_t indirectionLevel() const;
        const size_t referenceLevel() const;
        const size_t pointerLevel() const;

        /**
         * follows the same format as `AltaCore::AST::Type::modifiers`
         */
        std::vector<uint8_t> modifiers;

        /**
         * @brief Add a `ref` to this type
         */
        std::shared_ptr<Type> reference(bool force = false) const;
        /**
         * @brief Remove a `ref` from this type (if present)
         */
        std::shared_ptr<Type> dereference() const;
        /**
         * @brief Add a `ptr` to this type
         */
        std::shared_ptr<Type> point() const;
        /**
         * @brief Remove a `ptr` from this type (if present)
         */
        std::shared_ptr<Type> follow() const;
        /**
         * @brief Remove a `ref` or `ptr` from this type (if present)
         * Basically, decreases the indirection level by one, regardless of the kind of indirection
         */
        std::shared_ptr<Type> followBlindly() const;
        /**
         * Remove the leftmost (i.e. root) `const` modifier from this type (if present)
         */
        std::shared_ptr<Type> deconstify(bool full = false) const;
        /**
         * Completely removes all outer references
         */
        std::shared_ptr<Type> destroyReferences() const;

        inline std::shared_ptr<Type> destroyIndirection() const {
          auto result = followBlindly();
          while (result->indirectionLevel() > 0) {
            result = result->followBlindly();
          }
          return result;
        };

        std::shared_ptr<Type> makeOptional() const;

        size_t compatiblity(const Type& other) const;
        bool isExactlyCompatibleWith(const Type& other) const;
        bool isCompatibleWith(const Type& other) const;

        static inline std::vector<uint8_t> createModifierVector(std::vector<std::vector<TypeModifierFlag>> modifiers) {
          std::vector<uint8_t> result;
          for (auto& level: modifiers) {
            uint8_t levelNumber = 0;
            for (auto& modifier: level) {
              levelNumber |= (uint8_t)modifier;
            }
            result.push_back(levelNumber);
          }
          return result;
        };

        Type():
          ScopeItem("", {}),
          isAny(true)
          {};
        Type(NativeType nativeTypeName, std::vector<uint8_t> modifiers = {}, std::string userDefinedName = "");
        Type(std::shared_ptr<Type> returnType, std::vector<std::tuple<std::string, std::shared_ptr<Type>, bool, std::string>> parameters, std::vector<uint8_t> modifiers = {}, bool isRawFunction = true);
        Type(std::shared_ptr<Class> klass, std::vector<uint8_t> modifiers = {});
        Type(std::vector<std::shared_ptr<Type>> unionOf, std::vector<uint8_t> modifiers = {});
        Type(bool _isOptional, std::shared_ptr<Type> _optionalTarget, std::vector<uint8_t> _modifiers = {}):
          ScopeItem("", {}),
          isOptional(_isOptional),
          optionalTarget(_optionalTarget),
          modifiers(_modifiers),
          isNative(false)
          {};

        // operator for `isCompatibleWith`
        bool operator %(const Type& other) const;
        bool operator ==(const Type& other) const;

        inline bool isUnion() const {
          return unionOf.size() > 0;
        };

        inline bool isSigned() const {
          bool result = true;
          for (const auto& modifier: modifiers) {
            if (modifier & static_cast<uint8_t>(AltaCore::DET::TypeModifierFlag::Signed)) {
              result = true;
            }
            if (modifier & static_cast<uint8_t>(AltaCore::DET::TypeModifierFlag::Unsigned)) {
              result = false;
            }
            if (modifier & (static_cast<uint8_t>(AltaCore::DET::TypeModifierFlag::Pointer) | static_cast<uint8_t>(AltaCore::DET::TypeModifierFlag::Reference))) {
              break;
            }
          }
          return result;
        };

        inline bool isFloatingPoint() const {
          return isNative && (nativeTypeName == NativeType::Float || nativeTypeName == NativeType::Double);
        };

        inline size_t nativeTypeBits() const {
          if (nativeTypeName == NativeType::Float) {
            return 32;
          } else if (nativeTypeName == NativeType::Double) {
            return 64;
          } else if (nativeTypeName == NativeType::Bool) {
            return 1;
          } else if (nativeTypeName == NativeType::Byte) {
            return 8;
          } else if (nativeTypeName == NativeType::Void) {
            return 0;
          }

          uint8_t bits = 32;

          for (const auto& modifier: modifiers) {
            if (modifier & static_cast<uint8_t>(AltaCore::DET::TypeModifierFlag::Long)) {
              if (bits < 64) {
                bits *= 2;
              }
            }

            if (modifier & static_cast<uint8_t>(AltaCore::DET::TypeModifierFlag::Short)) {
              if (bits > 8) {
                bits /= 2;
              }
            }

            if (modifier & (static_cast<uint8_t>(AltaCore::DET::TypeModifierFlag::Pointer) | static_cast<uint8_t>(AltaCore::DET::TypeModifierFlag::Reference))) {
              break;
            }
          }

          return bits;
        };

        const size_t requiredArgumentCount() const;

        bool includes(const std::shared_ptr<Type> otherType) const;

        virtual std::string toString() const;
    };
  };
};

// std::hash implementation
namespace std {
  template<> struct hash<AltaCore::DET::Type> {
    size_t operator ()(const AltaCore::DET::Type& val) const {
      size_t result = 17;
      result = result * 31 + hash<bool>()(val.isAny);
      result = result * 31 + hash<bool>()(val.isNative);
      result = result * 31 + hash<bool>()(val.isFunction);
      result = result * 31 + hash<bool>()(val.isMethod);
      result = result * 31 + hash<bool>()(val.isAccessor);
      result = result * 31 + hash<int>()((int)val.nativeTypeName);
      result = result * 31 + hash<string>()(val.userDefinedName);
      for (auto& mod: val.modifiers) {
        result = result * 31 + hash<uint8_t>()(mod);
      }
      if (val.klass) {
        result = result * 31 + hash<string>()(val.klass->id);
      }
      if (val.returnType) {
        result = result * 31 + hash<AltaCore::DET::Type>()(*val.returnType);
      }
      for (auto& param: val.parameters) {
        auto& [id, type, isVariable, name] = param;
        result = result * 31 + hash<AltaCore::DET::Type>()(*type);
        result = result * 31 + hash<bool>()(isVariable);
        result = result * 31 + hash<string>()(name);
      }
      for (auto& item: val.unionOf) {
        result = result * 31 + hash<AltaCore::DET::Type>()(*item);
      }
      return result;
    };
  };
};

namespace AltaCore {
  namespace DET {
    struct TypePointerHash {
      size_t operator ()(const std::shared_ptr<AltaCore::DET::Type>& val) const {
        return std::hash<AltaCore::DET::Type>()(*val);
      };
    };
    struct TypePointerComparator {
      bool operator() (const std::shared_ptr<AltaCore::DET::Type>& lhs, const std::shared_ptr<AltaCore::DET::Type>& rhs) const {
        return (*lhs) == (*rhs);
      };
    };
  };
};

#endif // ALTACORE_DET_TYPE_HPP
