#ifndef ALTACORE_AST_FUNCTION_CALL_EXPRESSION_HPP
#define ALTACORE_AST_FUNCTION_CALL_EXPRESSION_HPP

#include "expression-node.hpp"
#include "../det/scope-item.hpp"
#include "../det/type.hpp"
#include <unordered_map>
#include "../variant.hpp"
#include "../det/function.hpp"

namespace AltaCore {
  namespace AST {
    class FunctionCallExpression: public ExpressionNode {
      public:
        virtual const NodeType nodeType();

        std::shared_ptr<ExpressionNode> target = nullptr;
        std::vector<std::pair<std::string, std::shared_ptr<ExpressionNode>>> arguments;

        FunctionCallExpression() {};
        FunctionCallExpression(std::shared_ptr<ExpressionNode> target, std::vector<std::pair<std::string, std::shared_ptr<ExpressionNode>>> arguments = {});

        static std::tuple<size_t, std::unordered_map<size_t, size_t>, std::vector<ALTACORE_VARIANT<std::pair<std::shared_ptr<ExpressionNode>, std::shared_ptr<DH::ExpressionNode>>, std::vector<std::pair<std::shared_ptr<ExpressionNode>, std::shared_ptr<DH::ExpressionNode>>>>>> findCompatibleCall(std::vector<std::tuple<std::string, std::shared_ptr<ExpressionNode>, std::shared_ptr<DH::ExpressionNode>>> arguments, std::vector<std::shared_ptr<DET::Type>> funcTypes);

        ALTACORE_AST_DETAIL(FunctionCallExpression);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_FUNCTION_CALL_EXPRESSION_HPP
