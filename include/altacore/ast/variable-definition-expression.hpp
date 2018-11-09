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
        Type* type;
        ExpressionNode* initializationExpression = nullptr;
        std::vector<std::string> modifiers;

        DET::Variable* $variable;

        VariableDefinitionExpression(std::string name, Type* type, ExpressionNode* initializationExpression);

        virtual void detail(DET::Scope* scope);
    };
  };
};

#endif // ALTACORE_AST_VARIABLE_DEFINITION_EXPRESSION_HPP