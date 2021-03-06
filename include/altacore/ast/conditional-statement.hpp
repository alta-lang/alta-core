#ifndef ALTACORE_AST_CONDITION_STATEMENT_HPP
#define ALTACORE_AST_CONDITION_STATEMENT_HPP

#include "expression-node.hpp"
#include "statement-node.hpp"
#include "../det/scope.hpp"
#include <vector>

namespace AltaCore {
  namespace AST {
    class ConditionalStatement: public StatementNode {
      public:
        virtual const NodeType nodeType();

        std::shared_ptr<ExpressionNode> primaryTest = nullptr;
        std::shared_ptr<StatementNode> primaryResult = nullptr;
        std::vector<std::pair<std::shared_ptr<ExpressionNode>, std::shared_ptr<StatementNode>>> alternatives;
        std::shared_ptr<StatementNode> finalResult = nullptr;

        ConditionalStatement() {};
        ConditionalStatement(std::shared_ptr<ExpressionNode> test, std::shared_ptr<StatementNode> primary, std::vector<std::pair<std::shared_ptr<ExpressionNode>, std::shared_ptr<StatementNode>>> alternatives = {}, std::shared_ptr<StatementNode> finalResult = nullptr);

        ALTACORE_AST_DETAIL(ConditionalStatement);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif /* ALTACORE_AST_CONDITION_STATEMENT_HPP */
