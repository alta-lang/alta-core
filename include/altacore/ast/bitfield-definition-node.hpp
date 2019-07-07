#ifndef ALTACORE_AST_BITFIELD_DEFINITION_NODE_HPP
#define ALTACORE_AST_BITFIELD_DEFINITION_NODE_HPP

#include "statement-node.hpp"
#include "type.hpp"
#include "attribute-node.hpp"

namespace AltaCore {
  namespace AST {
    class BitfieldDefinitionNode: public StatementNode, public std::enable_shared_from_this<BitfieldDefinitionNode> {
      public:
        virtual const NodeType nodeType();

        std::vector<std::string> modifiers;
        std::shared_ptr<Type> underlyingType;
        std::string name;
        std::vector<std::tuple<std::shared_ptr<Type>, std::string, size_t, size_t>> members;
        std::vector<std::shared_ptr<AttributeNode>> attributes;

        BitfieldDefinitionNode() {};

        ALTACORE_AST_DETAIL(BitfieldDefinitionNode);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_BITFIELD_DEFINITION_NODE_HPP
