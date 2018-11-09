#ifndef ALTACORE_DET_TYPE_HPP
#define ALTACORE_DET_TYPE_HPP

#include "node.hpp"
#include "../ast/expression-node.hpp"
#include <string>
#include <vector>

namespace AltaCore {
  namespace DET {
    class Type: public Node {
      public:
        virtual const NodeType nodeType();
        virtual Type* clone();
        virtual Type* deepClone();

        // why is this a static method and not a constructor?
        // because it can return `nullptr` if it doesn't find an
        // underlying type for the given expression, whereas with a
        // constructor, we can't do that
        static Type* getUnderlyingType(AST::ExpressionNode* expression);

        bool isNative = true;
        NativeType nativeTypeName = NativeType::Integer;

        /**
         * follows the same format as `AltaCore::AST::Type::modifiers`
         */
        std::vector<uint8_t> modifiers;
        /**
         * @brief Add a `ref` to this type
         */
        Type* reference();
        /**
         * @brief Remove a `ref` from this type (if present)
         */
        Type* dereference();
        /**
         * @brief Add a `ptr` to this type
         */
        Type* point();
        /**
         * @brief Remove a `ptr` from this type (if present)
         */
        Type* follow();
        /**
         * @brief Remove a `ref` or `ptr` from this type (if present)
         * Basically, decreases the indirection level by one, regardless of the kind of indirection
         */
        Type* followBlindly();

        Type(NativeType nativeTypeName, std::vector<uint8_t> modifiers);
    };
  };
};

#endif // ALTACORE_DET_TYPE_HPP