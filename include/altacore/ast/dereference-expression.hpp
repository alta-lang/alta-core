#ifndef ALTACORE_AST_DEREFERENCE_EXPRESSION_HPP
#define ALTACORE_AST_DEREFERENCE_EXPRESSION_HPP

#include "expression-node.hpp"
#include <string>

namespace AltaCore {
  namespace AST {
    class DereferenceExpression: public ExpressionNode {
      public:
        virtual const NodeType nodeType();

        std::shared_ptr<ExpressionNode> target = nullptr;

        DereferenceExpression() {};

        virtual void detail(std::shared_ptr<DET::Scope> scope);
    };
  };
};

#endif // ALTACORE_AST_BINARY_OPERATION_HPP
