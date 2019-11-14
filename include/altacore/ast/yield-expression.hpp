#ifndef ALTACORE_AST_YIELD_EXPRESSION_HPP
#define ALTACORE_AST_YIELD_EXPRESSION_HPP

#include "expression-node.hpp"
#include <string>

namespace AltaCore {
  namespace AST {
    class YieldExpression: public ExpressionNode {
      public:
        virtual const NodeType nodeType();

        std::shared_ptr<ExpressionNode> target = nullptr;

        YieldExpression() {};

        ALTACORE_AST_DETAIL(YieldExpression);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_YIELD_EXPRESSION_HPP
