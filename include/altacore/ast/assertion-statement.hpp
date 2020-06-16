#ifndef ALTACORE_AST_ASSERTION_STATEMENT_HPP
#define ALTACORE_AST_ASSERTION_STATEMENT_HPP

#include "statement-node.hpp"
#include "expression-node.hpp"

namespace AltaCore {
  namespace AST {
    class AssertionStatement: public StatementNode {
      public:
        virtual const NodeType nodeType();

        std::shared_ptr<AST::ExpressionNode> test = nullptr;

        AssertionStatement() {};

        ALTACORE_AST_DETAIL(AssertionStatement);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_ASSERTION_STATEMENT_HPP
