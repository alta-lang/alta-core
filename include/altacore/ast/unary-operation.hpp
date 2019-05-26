#ifndef ALTACORE_AST_UNARY_OPERATION_HPP
#define ALTACORE_AST_UNARY_OPERATION_HPP

#include "expression-node.hpp"
#include <string>

namespace AltaCore {
  namespace AST {
    class UnaryOperation: public ExpressionNode {
      public:
        virtual const NodeType nodeType();

        UOperatorType type = UOperatorType::Not;
        std::shared_ptr<ExpressionNode> target = nullptr;

        UnaryOperation() {};
        UnaryOperation(UOperatorType type, std::shared_ptr<ExpressionNode> target);

        ALTACORE_AST_DETAIL(UnaryOperation);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_UNARY_OPERATION_HPP
