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

        WhileLoopStatement() {};

        ALTACORE_AST_DETAIL(WhileLoopStatement);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_WHILE_LOOP_STATEMENT_HPP
