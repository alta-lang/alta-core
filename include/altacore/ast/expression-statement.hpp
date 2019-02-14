#ifndef ALTACORE_AST_EXPRESSION_STATEMENT_HPP
#define ALTACORE_AST_EXPRESSION_STATEMENT_HPP

#include "statement-node.hpp"
#include "expression-node.hpp"
#include "../det/scope.hpp"

namespace AltaCore {
  namespace AST {
    class ExpressionStatement: public StatementNode {
      public:
        virtual const NodeType nodeType();

        std::shared_ptr<ExpressionNode> expression = nullptr;

        ExpressionStatement();
        ExpressionStatement(std::shared_ptr<ExpressionNode> expression);

        ALTACORE_AST_DETAIL(ExpressionStatement);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_EXPRESSION_STATEMENT_HPP
