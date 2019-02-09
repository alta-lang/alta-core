#ifndef ALTACORE_AST_TYPE_ALIAS_STATEMENT_HPP
#define ALTACORE_AST_TYPE_ALIAS_STATEMENT_HPP

#include "statement-node.hpp"
#include "type.hpp"
#include <string>
#include <vector>

namespace AltaCore {
  namespace AST {
    class TypeAliasStatement: public StatementNode {
      public:
        virtual const NodeType nodeType();

        std::vector<std::string> modifiers;
        std::string name;
        std::shared_ptr<Type> type = nullptr;

        bool isExport = false;

        TypeAliasStatement() {};

        virtual void detail(std::shared_ptr<DET::Scope> scope);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_TYPE_ALIAS_STATEMENT_HPP
