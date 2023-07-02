#ifndef ALTACORE_AST_VOID_EXPRESSION_HPP
#define ALTACORE_AST_VOID_EXPRESSION_HPP

#include "expression-node.hpp"
#include <string>

namespace AltaCore {
  namespace AST {
    class VoidExpression: public ExpressionNode {
      public:
        virtual const NodeType nodeType();

        VoidExpression() {};

        ALTACORE_AST_AUTO_DETAIL(VoidExpression);
    };
  };
};

#endif // ALTACORE_AST_VOID_EXPRESSION_HPP
