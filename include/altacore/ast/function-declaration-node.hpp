#ifndef ALTACORE_AST_FUNCTION_DECLARATION_NODE_HPP
#define ALTACORE_AST_FUNCTION_DECLARATION_NODE_HPP

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
    class FunctionDeclarationNode: public StatementNode {
      public:
        virtual const NodeType nodeType();

        std::string name;
        std::vector<std::shared_ptr<Parameter>> parameters;
        std::shared_ptr<Type> returnType = nullptr;
        std::vector<std::string> modifiers;

        std::shared_ptr<DET::Function> $function = nullptr;

        FunctionDeclarationNode() {};
        FunctionDeclarationNode(std::string name, std::vector<std::shared_ptr<Parameter>> parameters, std::shared_ptr<Type> returnType, std::vector<std::string> modifiers);

        virtual void detail(std::shared_ptr<DET::Scope> scope);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_FUNCTION_DECLARATION_NODE_HPP
