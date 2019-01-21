#ifndef ALTACORE_AST_WHILE_LOOP_STATEMENT_HPP
#define ALTACORE_AST_WHILE_LOOP_STATEMENT_HPP

#include "statement-node.hpp"
#include "expression-node.hpp"
#include "../det/scope.hpp"
#include <string>
#include <vector>

namespace AltaCore {
  namespace AST {
    class WhileLoopStatement: public StatementNode {
      public:
        virtual const NodeType nodeType();

        std::shared_ptr<ExpressionNode> test = nullptr;
        std::shared_ptr<StatementNode> body = nullptr;

        std::shared_ptr<DET::Scope> $scope = nullptr;

        WhileLoopStatement() {};

        virtual void detail(std::shared_ptr<DET::Scope> scope);
    };
  };
};

#endif // ALTACORE_AST_WHILE_LOOP_STATEMENT_HPP
