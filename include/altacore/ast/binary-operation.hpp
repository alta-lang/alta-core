#ifndef ALTACORE_AST_BINARY_OPERATION_HPP
#define ALTACORE_AST_BINARY_OPERATION_HPP

#include "expression-node.hpp"
#include <string>

namespace AltaCore {
  namespace AST {
    // this is a binary operation, not in the sense
    // that it performs any binary manipulation, but in
    // literal sense of the word:
    //     bi(n)-ary = 2-arguments
    class BinaryOperation: public ExpressionNode {
      public:
        virtual const NodeType nodeType();

        OperatorType type = OperatorType::Addition;
        std::shared_ptr<ExpressionNode> left = nullptr;
        std::shared_ptr<ExpressionNode> right = nullptr;

        BinaryOperation() {};
        BinaryOperation(OperatorType type, std::shared_ptr<ExpressionNode> left, std::shared_ptr<ExpressionNode> right);

        ALTACORE_AST_DETAIL(BinaryOperation);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_BINARY_OPERATION_HPP
