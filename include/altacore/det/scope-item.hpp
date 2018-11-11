#ifndef ALTACORE_DET_SCOPE_ITEM_HPP
#define ALTACORE_DET_SCOPE_ITEM_HPP

#include "node.hpp"
#include "scope.hpp"
#include <string>

namespace AltaCore {
  namespace DET {
    class ScopeItem: public Node {
      public:
        virtual const NodeType nodeType();
        virtual std::shared_ptr<Node> clone();
        virtual std::shared_ptr<Node> deepClone();

        std::weak_ptr<Scope> parentScope;
        std::string name;

        ScopeItem(std::string name, std::shared_ptr<Scope> parentScope = nullptr);
    };
  };
};

#endif // ALTACORE_DET_SCOPE_ITEM_HPP
