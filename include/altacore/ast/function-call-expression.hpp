#ifndef ALTACORE_AST_FUNCTION_CALL_EXPRESSION_HPP
#define ALTACORE_AST_FUNCTION_CALL_EXPRESSION_HPP

#include "expression-node.hpp"
#include "../det/scope-item.hpp"
#include "../det/type.hpp"

namespace AltaCore {
  namespace AST {
    class FunctionCallExpression: public ExpressionNode {
      public:
        virtual const NodeType nodeType();

        std::shared_ptr<ExpressionNode> target;
        std::vector<std::shared_ptr<ExpressionNode>> arguments;

        std::shared_ptr<DET::Type> $targetType;

        FunctionCallExpression(std::shared_ptr<ExpressionNode> target, std::vector<std::shared_ptr<ExpressionNode>> arguments);
        
        virtual void detail(std::shared_ptr<DET::Scope> scope);
    };
  };
};

#endif // ALTACORE_AST_FUNCTION_CALL_EXPRESSION_HPP
