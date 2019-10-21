#ifndef ALTACORE_DET_ALIAS_HPP
#define ALTACORE_DET_ALIAS_HPP

#include "scope-item.hpp"
#include <string>

namespace AltaCore {
  namespace DET {
    class Alias: public ScopeItem {
      public:
        virtual const NodeType nodeType();
        virtual std::shared_ptr<Node> clone();
        virtual std::shared_ptr<Node> deepClone();

        std::shared_ptr<ScopeItem> target;

        Alias(std::string name, std::shared_ptr<ScopeItem> target, std::shared_ptr<Scope> parentScope = nullptr);

        virtual std::string toString() const;
    };
  };
};

#endif // ALTACORE_DET_ALIAS_HPP
