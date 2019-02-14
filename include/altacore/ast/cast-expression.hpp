#ifndef ALTACORE_AST_CAST_EXPRESSION_HPP
#define ALTACORE_AST_CAST_EXPRESSION_HPP

#include "expression-node.hpp"
#include "type.hpp"
#include <string>

namespace AltaCore {
  namespace AST {
    class CastExpression: public ExpressionNode {
    public:
      virtual const NodeType nodeType();

      std::shared_ptr<ExpressionNode> target = nullptr;
      std::shared_ptr<Type> type = nullptr;

      CastExpression();

      ALTACORE_AST_DETAIL(CastExpression);
      ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_CAST_EXPRESSION_HPP
