#ifndef ALTACORE_AST_VARIABLE_DECLARATION_STATEMENT_HPP
#define ALTACORE_AST_VARIABLE_DECLARATION_STATEMENT_HPP

#include "statement-node.hpp"
#include "type.hpp"
#include "../det/variable.hpp"
#include "attribute-node.hpp"
#include <string>
#include <vector>

namespace AltaCore {
  namespace AST {
    class VariableDeclarationStatement: public StatementNode, public std::enable_shared_from_this<VariableDeclarationStatement> {
      public:
        virtual const NodeType nodeType();

        std::string name;
        std::shared_ptr<Type> type = nullptr;
        std::vector<std::string> modifiers;
        std::vector<std::shared_ptr<AttributeNode>> attributes;

        VariableDeclarationStatement() {};

        ALTACORE_AST_DETAIL(VariableDeclarationStatement);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_VARIABLE_DECLARATION_STATEMENT_HPP
