#ifndef ALTACORE_AST_FETCH_HPP
#define ALTACORE_AST_FETCH_HPP

#include "expression-node.hpp"
#include "../det/scope-item.hpp"
#include "../det/type.hpp"

namespace AltaCore {
  namespace AST {
    class Fetch: public ExpressionNode {
      public:
        virtual const NodeType nodeType();

        std::string query;

        std::vector<std::shared_ptr<DET::ScopeItem>> $items;
        std::shared_ptr<DET::ScopeItem> $narrowedTo;

        Fetch(std::string query);

        void narrowTo(std::shared_ptr<DET::Type> type);
        virtual void detail(std::shared_ptr<DET::Scope> scope);
    };
  };
};

#endif // ALTACORE_AST_FETCH_HPP
