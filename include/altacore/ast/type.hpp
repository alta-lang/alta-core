#ifndef ALTACORE_AST_TYPE_HPP
#define ALTACORE_AST_TYPE_HPP

#include "node.hpp"
#include "../det/scope.hpp"
#include "../det/type.hpp"
#include <vector>
#include <string>

namespace AltaCore {
  namespace AST {
    class Type: public Node {
      public:
        virtual const NodeType nodeType();

        std::string name;
        /**
         * identifies the proper association of modifiers with each other
         * including indirection. each member is a bitset for the modifiers of that layer.
         * the layers are organized logically (e.g. a constant pointer to a pointer to an int would
         * be [TypeModifierFlag::Constant | TypeModifierFlag::Pointer, TypeModifierFlag::Pointer])
         */
        std::vector<uint8_t> modifiers;

        DET::Type* $type;

        Type(std::string name, std::vector<uint8_t> modifiers);

        virtual void detail(DET::Scope* scope);
    };
  };
};

#endif // ALTACORE_AST_TYPE_NODE_HPP