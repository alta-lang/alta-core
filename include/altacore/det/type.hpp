#ifndef ALTACORE_DET_TYPE_HPP
#define ALTACORE_DET_TYPE_HPP

#include "node.hpp"
#include "../ast/expression-node.hpp"
#include <string>
#include <vector>

namespace AltaCore {
  namespace DET {
    class Type: public Node, public std::enable_shared_from_this<Type> {
      public:
        virtual const NodeType nodeType();
        virtual std::shared_ptr<Node> clone();
        virtual std::shared_ptr<Node> deepClone();

        // why is this a static method and not a constructor?
        // because it can return `nullptr` if it doesn't find an
        // underlying type for the given expression, whereas with a
        // constructor, we can't do that
        static std::shared_ptr<Type> getUnderlyingType(AST::ExpressionNode* expression);

        bool isNative = true;
        NativeType nativeTypeName = NativeType::Integer;

        /**
         * follows the same format as `AltaCore::AST::Type::modifiers`
         */
        std::vector<uint8_t> modifiers;
        /**
         * @brief Add a `ref` to this type
         */
        std::shared_ptr<Type> reference();
        /**
         * @brief Remove a `ref` from this type (if present)
         */
        std::shared_ptr<Type> dereference();
        /**
         * @brief Add a `ptr` to this type
         */
        std::shared_ptr<Type> point();
        /**
         * @brief Remove a `ptr` from this type (if present)
         */
        std::shared_ptr<Type> follow();
        /**
         * @brief Remove a `ref` or `ptr` from this type (if present)
         * Basically, decreases the indirection level by one, regardless of the kind of indirection
         */
        std::shared_ptr<Type> followBlindly();

        bool isCompatiableWith(const Type& other);

        Type(NativeType nativeTypeName, std::vector<uint8_t> modifiers = {});

        // operator for `isCompatiableWith`
        bool operator %(const Type& other);
    };
  };
};

#endif // ALTACORE_DET_TYPE_HPP
