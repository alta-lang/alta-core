#ifndef ALTACORE_AST_AWAIT_EXPRESSION_HPP
#define ALTACORE_AST_AWAIT_EXPRESSION_HPP

#include "expression-node.hpp"

namespace AltaCore {
  namespace AST {
    class AwaitExpression: public ExpressionNode {
      public:
        virtual const NodeType nodeType();

        std::shared_ptr<ExpressionNode> target = nullptr;

        AwaitExpression() {};

        ALTACORE_AST_DETAIL(AwaitExpression);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_AWAIT_EXPRESSION_HPP
