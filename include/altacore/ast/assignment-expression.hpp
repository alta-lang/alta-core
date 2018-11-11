#ifndef ALTACORE_AST_ASSIGNMENT_EXPRESSION_HPP
#define ALTACORE_AST_ASSIGNMENT_EXPRESSION_HPP

#include "expression-node.hpp"

namespace AltaCore {
  namespace AST {
    class AssignmentExpression: public ExpressionNode {
      public:
        virtual const NodeType nodeType();

        std::shared_ptr<AST::ExpressionNode> target;
        std::shared_ptr<AST::ExpressionNode> value;

        AssignmentExpression(std::shared_ptr<AST::ExpressionNode> target, std::shared_ptr<AST::ExpressionNode> value);

        virtual void detail(std::shared_ptr<DET::Scope> scope);
    };
  };
};

#endif // ALTACORE_AST_ASSIGNMENT_EXPRESSION_HPP
