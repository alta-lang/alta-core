#ifndef ALTACORE_AST_SUBSCRIPT_EXPRESSION_HPP
#define ALTACORE_AST_SUBSCRIPT_EXPRESSION_HPP

#include "expression-node.hpp"

namespace AltaCore {
  namespace AST {
    class SubscriptExpression: public ExpressionNode {
      public:
        virtual const NodeType nodeType();

        static const std::shared_ptr<DET::Type> sizeType;

        std::shared_ptr<AST::ExpressionNode> target = nullptr;
        std::shared_ptr<AST::ExpressionNode> index = nullptr;

        SubscriptExpression() {};
        SubscriptExpression(std::shared_ptr<AST::ExpressionNode> target, std::shared_ptr<AST::ExpressionNode> index);

        ALTACORE_AST_DETAIL(SubscriptExpression);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_SUBSCRIPT_EXPRESSION_HPP
