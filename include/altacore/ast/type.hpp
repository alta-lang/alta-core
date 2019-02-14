#ifndef ALTACORE_AST_TYPE_HPP
#define ALTACORE_AST_TYPE_HPP

#include "node.hpp"
#include "attribute-node.hpp"
#include "../det/scope.hpp"
#include "../det/type.hpp"
#include "expression-node.hpp"
#include <vector>
#include <string>

namespace AltaCore {
  namespace AST {
    class Type: public Node {
      public:
        virtual const NodeType nodeType();

        bool isAny = false;
        bool isFunction = false;
        std::shared_ptr<Type> returnType = nullptr;
        std::vector<std::tuple<std::shared_ptr<Type>, bool, std::string>> parameters;
        std::string name;
        /**
         * identifies the proper association of modifiers with each other
         * including indirection. each member is a bitset for the modifiers of that layer.
         * the layers are organized logically (e.g. a constant pointer to a pointer to an int would
         * be [TypeModifierFlag::Constant | TypeModifierFlag::Pointer, TypeModifierFlag::Pointer])
         */
        std::vector<uint8_t> modifiers;
        bool isNative = true;
        std::shared_ptr<ExpressionNode> lookup = nullptr;

        Type() {};
        Type(std::string name, std::vector<uint8_t> modifiers);
        Type(std::shared_ptr<Type> returnType, std::vector<std::tuple<std::shared_ptr<Type>, bool, std::string>> parameters, std::vector<uint8_t> modifiers);

        virtual std::shared_ptr<DH::Node> detail(std::shared_ptr<DET::Scope> scope, bool hoist = true);
        std::shared_ptr<DH::Type> fullDetail(std::shared_ptr<DET::Scope> scope, bool hoist = true) {
          return std::dynamic_pointer_cast<DH::Type>(detail(scope));
        };
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_TYPE_NODE_HPP
