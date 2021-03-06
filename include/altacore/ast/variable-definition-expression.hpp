#ifndef ALTACORE_AST_VARIABLE_DEFINITION_EXPRESSION_HPP
#define ALTACORE_AST_VARIABLE_DEFINITION_EXPRESSION_HPP

#include "expression-node.hpp"
#include "type.hpp"
#include "../det/variable.hpp"
#include <string>
#include <vector>

namespace AltaCore {
  namespace AST {
    class VariableDefinitionExpression: public ExpressionNode {
      public:
        virtual const NodeType nodeType();

        std::string name;
        std::shared_ptr<Type> type = nullptr;
        std::shared_ptr<ExpressionNode> initializationExpression = nullptr;
        std::vector<std::string> modifiers;

        VariableDefinitionExpression() {};
        VariableDefinitionExpression(std::string name, std::shared_ptr<Type> type, std::shared_ptr<ExpressionNode> initializationExpression);

        ALTACORE_AST_DETAIL_NO_BODY_OPT(VariableDefinitionExpression);
        ALTACORE_AST_INFO_DETAIL(VariableDefinitionExpression);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_VARIABLE_DEFINITION_EXPRESSION_HPP
