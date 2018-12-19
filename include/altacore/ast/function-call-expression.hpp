#ifndef ALTACORE_AST_FUNCTION_CALL_EXPRESSION_HPP
#define ALTACORE_AST_FUNCTION_CALL_EXPRESSION_HPP

#include "expression-node.hpp"
#include "../det/scope-item.hpp"
#include "../det/type.hpp"
#include <unordered_map>
#include "../variant.hpp"

namespace AltaCore {
  namespace AST {
    class FunctionCallExpression: public ExpressionNode {
      public:
        virtual const NodeType nodeType();

        std::shared_ptr<ExpressionNode> target = nullptr;
        std::vector<std::pair<std::string, std::shared_ptr<ExpressionNode>>> arguments;

        std::shared_ptr<DET::Type> $targetType = nullptr;
        std::unordered_map<size_t, size_t> $argumentMap;
        std::vector<ALTACORE_VARIANT<std::shared_ptr<ExpressionNode>, std::vector<std::shared_ptr<ExpressionNode>>>> $adjustedArguments;

        FunctionCallExpression() {};
        FunctionCallExpression(std::shared_ptr<ExpressionNode> target, std::vector<std::pair<std::string, std::shared_ptr<ExpressionNode>>> arguments = {});

        virtual void detail(std::shared_ptr<DET::Scope> scope);
    };
  };
};

#endif // ALTACORE_AST_FUNCTION_CALL_EXPRESSION_HPP
