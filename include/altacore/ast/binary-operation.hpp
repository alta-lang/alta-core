#ifndef ALTACORE_AST_BINARY_OPERATION_HPP
#define ALTACORE_AST_BINARY_OPERATION_HPP

#include "expression-node.hpp"
#include <string>

namespace AltaCore {
  namespace AST {
    class BinaryOperation: public ExpressionNode {
      public:
        virtual const NodeType nodeType();

        OperatorType type;
        std::shared_ptr<ExpressionNode> left;
        std::shared_ptr<ExpressionNode> right;

        BinaryOperation(OperatorType type, std::shared_ptr<ExpressionNode> left, std::shared_ptr<ExpressionNode> right);

        virtual void detail(std::shared_ptr<DET::Scope> scope);
    };
  };
};

#endif // ALTACORE_AST_BINARY_OPERATION_HPP
