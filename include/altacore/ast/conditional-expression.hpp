#ifndef ALTACORE_AST_CONDITION_EXPRESSION_HPP
#define ALTACORE_AST_CONDITION_EXPRESSION_HPP

#include "expression-node.hpp"

namespace AltaCore {
  namespace AST {
    class ConditionalExpression: public ExpressionNode {
      public:
        virtual const NodeType nodeType();

        std::shared_ptr<ExpressionNode> test = nullptr;
        std::shared_ptr<ExpressionNode> primaryResult = nullptr;
        std::shared_ptr<ExpressionNode> secondaryResult = nullptr;

        ConditionalExpression() {};
        ConditionalExpression(std::shared_ptr<ExpressionNode> test, std::shared_ptr<ExpressionNode> primary, std::shared_ptr<ExpressionNode> secondary);

        ALTACORE_AST_DETAIL(ConditionalExpression);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif /* ALTACORE_AST_CONDITION_EXPRESSION_HPP */
