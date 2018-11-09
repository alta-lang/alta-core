#ifndef ALTACORE_AST_RETURN_DIRECTIVE_NODE_HPP
#define ALTACORE_AST_RETURN_DIRECTIVE_NODE_HPP

#include "statement-node.hpp"
#include "expression-node.hpp"
#include "../det/scope.hpp"
#include <string>
#include <vector>

namespace AltaCore {
  namespace AST {
    class ReturnDirectiveNode: public StatementNode {
      public:
        virtual const NodeType nodeType();

        ExpressionNode* expression = nullptr;

        ReturnDirectiveNode(ExpressionNode* expression);

        virtual void detail(DET::Scope* scope);
    };
  };
};

#endif // ALTACORE_AST_RETURN_DIRECTIVE_NODE_HPP