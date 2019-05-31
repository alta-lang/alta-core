#ifndef ALTACORE_AST_DELETE_STATEMENT_HPP
#define ALTACORE_AST_DELETE_STATEMENT_HPP

#include "statement-node.hpp"
#include "expression-node.hpp"

namespace AltaCore {
  namespace AST {
    class DeleteStatement: public StatementNode {
      public:
        virtual const NodeType nodeType();

        DeleteStatement() {};

        bool persistent = false;
        std::shared_ptr<ExpressionNode> target = nullptr;

        ALTACORE_AST_DETAIL(DeleteStatement);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_DELETE_STATEMENT_HPP
