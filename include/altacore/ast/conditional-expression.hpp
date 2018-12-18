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

        virtual void detail(std::shared_ptr<DET::Scope> scope);
    };
  };
};

#endif /* ALTACORE_AST_CONDITION_EXPRESSION_HPP */
