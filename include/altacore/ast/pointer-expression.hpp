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

        ALTACORE_AST_DETAIL(PointerExpression);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_BINARY_OPERATION_HPP
