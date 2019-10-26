#ifndef ALTACORE_AST_ENUMERATION_DEFINITION_NODE_HPP
#define ALTACORE_AST_ENUMERATION_DEFINITION_NODE_HPP

#include "statement-node.hpp"
#include "expression-node.hpp"
#include "type.hpp"

namespace AltaCore {
  namespace AST {
    class EnumerationDefinitionNode: public StatementNode {
      public:
        virtual const NodeType nodeType();

        std::vector<std::string> modifiers;
        std::string name;
        std::shared_ptr<Type> underlyingType;
        std::vector<std::pair<std::string, std::shared_ptr<ExpressionNode>>> members;

        EnumerationDefinitionNode() {};

        ALTACORE_AST_DETAIL(EnumerationDefinitionNode);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_ENUMERATION_DEFINITION_NODE_HPP
