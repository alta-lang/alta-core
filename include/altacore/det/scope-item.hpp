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
        virtual ScopeItem* clone();
        virtual ScopeItem* deepClone();

        Scope* parentScope;
        std::string name;

        ScopeItem(std::string name, Scope* parentScope = nullptr);
    };
  };
};

#endif // ALTACORE_DET_SCOPE_ITEM_HPP