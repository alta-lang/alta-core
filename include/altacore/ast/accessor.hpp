#ifndef ALTACORE_AST_ACCESSOR_HPP
#define ALTACORE_AST_ACCESSOR_HPP

#include "expression-node.hpp"
#include "../det/scope-item.hpp"

namespace AltaCore {
  namespace AST {
    class Accessor: public ExpressionNode {
      public:
        virtual const NodeType nodeType();

        AST::ExpressionNode* target;
        std::string query;

        DET::ScopeItem* $item;

        Accessor(AST::ExpressionNode* target, std::string query);

        virtual void detail(DET::Scope* scope);
    };
  };
};

#endif // ALTACORE_AST_ACCESSOR_HPP