#ifndef ALTACORE_AST_STRUCTURE_DEFINITION_NODE_HPP
#define ALTACORE_AST_STRUCTURE_DEFINITION_NODE_HPP

#include "statement-node.hpp"
#include "type.hpp"
#include "attribute-node.hpp"

namespace AltaCore {
  namespace AST {
    class StructureDefinitionStatement: public StatementNode, public std::enable_shared_from_this<StructureDefinitionStatement> {
      public:
        bool isExternal = false;
        bool isTyped = false;

        virtual const NodeType nodeType();

        std::vector<std::string> modifiers;
        std::string name;
        std::vector<std::pair<std::shared_ptr<Type>, std::string>> members;
        std::vector<std::shared_ptr<AttributeNode>> attributes;

        StructureDefinitionStatement() {};
        StructureDefinitionStatement(std::string name);

        ALTACORE_AST_DETAIL(StructureDefinitionStatement);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_STRUCTURE_DEFINITION_NODE_HPP
