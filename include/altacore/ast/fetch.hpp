#ifndef ALTACORE_AST_FETCH_HPP
#define ALTACORE_AST_FETCH_HPP

#include "expression-node.hpp"
#include "../det/scope-item.hpp"

namespace AltaCore {
  namespace AST {
    class Fetch: public ExpressionNode {
      public:
        virtual const NodeType nodeType();

        std::string query;

        DET::ScopeItem* $item;

        Fetch(std::string query);
        
        virtual void detail(DET::Scope* scope);
    };
  };
};

#endif // ALTACORE_AST_FETCH_HPP