#ifndef ALTACORE_AST_TYPE_ALIAS_STATEMENT_HPP
#define ALTACORE_AST_TYPE_ALIAS_STATEMENT_HPP

#include "statement-node.hpp"
#include "type.hpp"
#include <string>
#include <vector>

namespace AltaCore {
  namespace AST {
    class TypeAliasStatement: public StatementNode, public std::enable_shared_from_this<TypeAliasStatement> {
      public:
        virtual const NodeType nodeType();

        std::vector<std::string> modifiers;
        std::string name;
        std::shared_ptr<Type> type = nullptr;
        std::vector<std::shared_ptr<AttributeNode>> attributes;

        TypeAliasStatement() {};

        ALTACORE_AST_DETAIL(TypeAliasStatement);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_TYPE_ALIAS_STATEMENT_HPP
