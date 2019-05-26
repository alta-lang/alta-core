#ifndef ALTACORE_AST_FOR_LOOP_STATEMENT_HPP
#define ALTACORE_AST_FOR_LOOP_STATEMENT_HPP

#include "statement-node.hpp"
#include "expression-node.hpp"
#include "../det/scope.hpp"
#include <string>
#include <vector>

namespace AltaCore {
  namespace AST {
    class ForLoopStatement: public StatementNode {
      public:
        virtual const NodeType nodeType();

        std::shared_ptr<ExpressionNode> initializer = nullptr;
        std::shared_ptr<ExpressionNode> condition = nullptr;
        std::shared_ptr<ExpressionNode> increment = nullptr;
        std::shared_ptr<StatementNode> body = nullptr;

        ForLoopStatement() {};

        ALTACORE_AST_DETAIL(ForLoopStatement);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_FOR_LOOP_STATEMENT_HPP
