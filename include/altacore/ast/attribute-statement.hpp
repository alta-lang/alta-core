#ifndef ALTACORE_AST_ATTRIBUTE_STATEMENT_HPP
#define ALTACORE_AST_ATTRIBUTE_STATEMENT_HPP

#include "statement-node.hpp"
#include "attribute-node.hpp"

namespace AltaCore {
  namespace AST {
    class AttributeStatement: public StatementNode {
      public:
        virtual const NodeType nodeType();

        std::shared_ptr<AttributeNode> attribute;

        AttributeStatement(std::shared_ptr<AttributeNode> attribute);

        virtual void detail(std::shared_ptr<DET::Scope> scope);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_ATTRIBUTE_STATEMENT_HPP
