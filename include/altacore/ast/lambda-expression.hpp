#ifndef ALTACORE_AST_LAMBDA_EXPRESSION_HPP
#define ALTACORE_AST_LAMBDA_EXPRESSION_HPP

#include "expression-node.hpp"
#include "type.hpp"
#include "block-node.hpp"
#include "parameter.hpp"
#include <string>

namespace AltaCore {
  namespace AST {
    class LambdaExpression: public ExpressionNode, public std::enable_shared_from_this<LambdaExpression> {
      public:
        virtual const NodeType nodeType();

        std::vector<std::shared_ptr<Parameter>> parameters;
        std::shared_ptr<Type> returnType = nullptr;

        std::vector<std::string> modifiers;
        std::vector<std::shared_ptr<AttributeNode>> attributes;

        std::shared_ptr<BlockNode> body = nullptr;

        LambdaExpression() {};

        ALTACORE_AST_DETAIL(LambdaExpression);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_LAMBDA_EXPRESSION_HPP