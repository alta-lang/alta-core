#ifndef ALTACORE_AST_POINTER_EXPRESSION_HPP
#define ALTACORE_AST_POINTER_EXPRESSION_HPP

#include "expression-node.hpp"
#include <string>

namespace AltaCore {
  namespace AST {
    class PointerExpression: public ExpressionNode {
      public:
        virtual const NodeType nodeType();

        std::shared_ptr<ExpressionNode> target = nullptr;

        PointerExpression() {};

        virtual void detail(std::shared_ptr<DET::Scope> scope);
    };
  };
};

#endif // ALTACORE_AST_BINARY_OPERATION_HPP
