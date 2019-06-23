#ifndef ALTACORE_AST_NULLPTR_EXPRESSION_HPP
#define ALTACORE_AST_NULLPTR_EXPRESSION_HPP

#include "expression-node.hpp"

namespace AltaCore {
  namespace AST {
    class NullptrExpression: public ExpressionNode {
      public:
        virtual const NodeType nodeType();

        NullptrExpression() {};

        ALTACORE_AST_AUTO_DETAIL(NullptrExpression);
    };
  };
};

#endif // ALTACORE_AST_NULLPTR_EXPRESSION_HPP
