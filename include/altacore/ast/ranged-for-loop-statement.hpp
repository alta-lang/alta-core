#ifndef ALTACORE_AST_RANGED_FOR_LOOP_STATEMENT_HPP
#define ALTACORE_AST_RANGED_FOR_LOOP_STATEMENT_HPP

#include "statement-node.hpp"
#include "expression-node.hpp"
#include "../det/scope.hpp"
#include "type.hpp"
#include <string>
#include <vector>

namespace AltaCore {
  namespace AST {
    class RangedForLoopStatement: public StatementNode {
      public:
        virtual const NodeType nodeType();

        std::string counterName;
        std::shared_ptr<Type> counterType = nullptr;
        std::shared_ptr<ExpressionNode> start = nullptr;
        std::shared_ptr<ExpressionNode> end = nullptr;
        std::shared_ptr<StatementNode> body = nullptr;

        bool decrement = false;
        bool inclusive = false;

        RangedForLoopStatement() {};

        ALTACORE_AST_DETAIL(RangedForLoopStatement);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_RANGED_FOR_LOOP_STATEMENT_HPP
