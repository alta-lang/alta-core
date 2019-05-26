#ifndef ALTACORE_DET_TYPE_HPP
#define ALTACORE_DET_TYPE_HPP

#include "scope-item.hpp"
#include <string>
#include <vector>

namespace AltaCore {
  namespace AST {
    class ExpressionNode;
  };
  namespace DET {
    class Type: public ScopeItem {
      private:
        bool commonCompatiblity(const Type& other);
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

        bool isAny = false;
        bool isNative = true;
        bool isFunction = false;
        bool isMethod = false;
        bool isAccessor = false;
        std::shared_ptr<Class> methodParent = nullptr;
        NativeType nativeTypeName = NativeType::Void;
        std::string userDefinedName;
        std::shared_ptr<Class> klass = nullptr;
        std::shared_ptr<Type> returnType = nullptr;
        std::vector<std::tuple<std::string, std::shared_ptr<Type>, bool, std::string>> parameters;

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
        std::shared_ptr<Type> reference() const;
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
        std::shared_ptr<Type> deconstify() const;
        /**
         * Completely removes all outer references
         */
        std::shared_ptr<Type> destroyReferences() const;

        size_t compatiblity(const Type& other);
        bool isExactlyCompatibleWith(const Type& other);
        bool isCompatibleWith(const Type& other);

        Type():
          ScopeItem(""),
          isAny(true)
          {};
        Type(NativeType nativeTypeName, std::vector<uint8_t> modifiers = {}, std::string userDefinedName = "");
        Type(std::shared_ptr<Type> returnType, std::vector<std::tuple<std::string, std::shared_ptr<Type>, bool, std::string>> parameters, std::vector<uint8_t> modifiers = {});
        Type(std::shared_ptr<Class> klass, std::vector<uint8_t> modifiers = {});

        // operator for `isCompatiableWith`
        bool operator %(const Type& other);
        
        const size_t requiredArgumentCount() const;
    };
  };
};

#endif // ALTACORE_DET_TYPE_HPP
