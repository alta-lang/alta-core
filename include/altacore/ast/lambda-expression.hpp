#ifndef ALTACORE_AST_LAMBDA_EXPRESSION_HPP
#define ALTACORE_AST_LAMBDA_EXPRESSION_HPP

#include "expression-node.hpp"
#include "type.hpp"
#include "block-node.hpp"
#include "parameter.hpp"
#include <string>

namespace AltaCore {
  namespace AST {
    class LambdaExpression: public ExpressionNode, public inheritable_enable_shared_from_this<LambdaExpression> {
      public:
        virtual const NodeType nodeType();

        std::vector<std::shared_ptr<Parameter>> parameters;
        std::shared_ptr<Type> returnType = nullptr;
        std::shared_ptr<Type> generatorParameter = nullptr;

        std::vector<std::string> modifiers;

        std::shared_ptr<BlockNode> body = nullptr;

        bool isGenerator = false;
        bool isAsync = false;

        LambdaExpression() {};

        ALTACORE_AST_DETAIL(LambdaExpression);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_LAMBDA_EXPRESSION_HPP
