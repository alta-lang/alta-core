#ifndef ALTACORE_AST_FUNCTION_DEFINITION_NODE_HPP
#define ALTACORE_AST_FUNCTION_DEFINITION_NODE_HPP

#include "statement-node.hpp"
#include "block-node.hpp"
#include "parameter.hpp"
#include "type.hpp"
#include "../det/function.hpp"
#include "attribute-node.hpp"
#include "../det/scope.hpp"
#include "generic.hpp"
#include <vector>
#include <string>

namespace AltaCore {
  namespace AST {
    class FunctionDefinitionNode: public StatementNode, public std::enable_shared_from_this<FunctionDefinitionNode> {
      public:
        virtual const NodeType nodeType();

        std::string name;
        std::vector<std::shared_ptr<Parameter>> parameters;
        std::shared_ptr<Type> returnType = nullptr;
        std::vector<std::string> modifiers;
        std::shared_ptr<BlockNode> body = nullptr;
        std::vector<std::shared_ptr<AttributeNode>> attributes;
        std::vector<std::shared_ptr<Generic>> generics;

        FunctionDefinitionNode() {};
        FunctionDefinitionNode(std::string name, std::vector<std::shared_ptr<Parameter>> parameters, std::shared_ptr<Type> returnType, std::vector<std::string> modifiers, std::shared_ptr<BlockNode> body);

        std::shared_ptr<DET::Function> instantiateGeneric(std::shared_ptr<DH::FunctionDefinitionNode> info, std::vector<std::shared_ptr<DET::Type>> genericArguments);

        ALTACORE_AST_DETAIL_NO_BODY_OPT(FunctionDefinitionNode);
        ALTACORE_AST_INFO_DETAIL(FunctionDefinitionNode);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_FUNCTION_DEFINITION_NODE_HPP
