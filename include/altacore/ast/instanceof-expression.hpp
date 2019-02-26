#ifndef ALTACORE_AST_INSTANCEOF_EXPRESSION_HPP
#define ALTACORE_AST_INSTANCEOF_EXPRESSION_HPP

#include "expression-node.hpp"
#include "type.hpp"

namespace AltaCore {
  namespace AST {
    class InstanceofExpression: public ExpressionNode {
      public:
        virtual const NodeType nodeType();

        std::shared_ptr<AST::ExpressionNode> target;
        std::shared_ptr<AST::Type> type;

        InstanceofExpression() {};
        InstanceofExpression(std::shared_ptr<AST::ExpressionNode> target, std::shared_ptr<AST::Type> type);

        ALTACORE_AST_DETAIL(InstanceofExpression);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_ASSIGNMENT_EXPRESSION_HPP
