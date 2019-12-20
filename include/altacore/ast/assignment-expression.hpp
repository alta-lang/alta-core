#ifndef ALTACORE_AST_ASSIGNMENT_EXPRESSION_HPP
#define ALTACORE_AST_ASSIGNMENT_EXPRESSION_HPP

#include "expression-node.hpp"
#include "attribute-node.hpp"

namespace AltaCore {
  namespace AST {
    using Shared::AssignmentType;

    class AssignmentExpression: public ExpressionNode, public inheritable_enable_shared_from_this<AssignmentExpression> {
      public:
        virtual const NodeType nodeType();

        std::shared_ptr<AST::ExpressionNode> target;
        std::shared_ptr<AST::ExpressionNode> value;
        AssignmentType type = AssignmentType::Simple;

        AssignmentExpression() {};
        AssignmentExpression(std::shared_ptr<AST::ExpressionNode> target, std::shared_ptr<AST::ExpressionNode> value);

        ALTACORE_AST_DETAIL(AssignmentExpression);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_ASSIGNMENT_EXPRESSION_HPP
