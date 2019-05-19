#ifndef ALTACORE_AST_ALIAS_STATEMENT_HPP
#define ALTACORE_AST_ALIAS_STATEMENT_HPP

#include "statement-node.hpp"
#include "retrieval-node.hpp"
#include "../det/scope-item.hpp"

namespace AltaCore {
  namespace AST {
    class AliasStatement: public StatementNode {
      public:
        virtual const NodeType nodeType();

        std::shared_ptr<AST::RetrievalNode> target = nullptr;
        std::string name;

        AliasStatement() {};

        ALTACORE_AST_DETAIL(AliasStatement);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_ALIAS_STATEMENT_HPP
