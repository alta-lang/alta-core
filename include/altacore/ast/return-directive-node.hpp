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

        std::shared_ptr<ExpressionNode> expression = nullptr;

        ReturnDirectiveNode(std::shared_ptr<ExpressionNode> expression);

        virtual void detail(std::shared_ptr<DET::Scope> scope);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_RETURN_DIRECTIVE_NODE_HPP
