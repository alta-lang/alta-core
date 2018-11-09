#ifndef ALTACORE_AST_FUNCTION_DEFINITION_NODE_HPP
#define ALTACORE_AST_FUNCTION_DEFINITION_NODE_HPP

#include "statement-node.hpp"
#include "block-node.hpp"
#include "parameter.hpp"
#include "type.hpp"
#include "../det/function.hpp"
#include "../det/scope.hpp"
#include <vector>
#include <string>

namespace AltaCore {
  namespace AST {
    class FunctionDefinitionNode: public StatementNode {
      public:
        virtual const NodeType nodeType();

        std::string name;
        std::vector<Parameter*> parameters;
        Type* returnType;
        std::vector<std::string> modifiers;
        BlockNode* body;

        DET::Function* $function;

        FunctionDefinitionNode(std::string name, std::vector<Parameter*> parameters, Type* returnType, std::vector<std::string> modifiers, BlockNode* body);

        virtual void detail(DET::Scope* scope);
    };
  };
};

#endif // ALTACORE_AST_FUNCTION_DEFINITION_NODE_HPP