#ifndef ALTACORE_AST_THROW_STATEMENT_HPP
#define ALTACORE_AST_THROW_STATEMENT_HPP

#include "statement-node.hpp"
#include "expression-node.hpp"
#include "../det/scope.hpp"
#include "../det/type.hpp"
#include <string>
#include <vector>

namespace AltaCore {
  namespace AST {
    class ThrowStatement: public StatementNode {
      public:
        virtual const NodeType nodeType();

        std::shared_ptr<ExpressionNode> expression = nullptr;

        ThrowStatement() {};

        ALTACORE_AST_DETAIL(ThrowStatement);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_THROW_STATEMENT_HPP
