#ifndef ALTACORE_AST_ACCESSOR_HPP
#define ALTACORE_AST_ACCESSOR_HPP

#include "expression-node.hpp"
#include "../det/scope-item.hpp"

namespace AltaCore {
  namespace AST {
    class Accessor: public ExpressionNode {
      public:
        virtual const NodeType nodeType();

        std::shared_ptr<AST::ExpressionNode> target;
        std::string query;

        std::shared_ptr<DET::ScopeItem> $item;

        Accessor(std::shared_ptr<AST::ExpressionNode> target, std::string query);

        virtual void detail(std::shared_ptr<DET::Scope> scope);
    };
  };
};

#endif // ALTACORE_AST_ACCESSOR_HPP
