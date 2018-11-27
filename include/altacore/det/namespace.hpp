#ifndef ALTACORE_DET_NAMESPACE_HPP
#define ALTACORE_DET_NAMESPACE_HPP

#include "scope.hpp"
#include "type.hpp"
#include <string>

namespace AltaCore {
  namespace DET {
    class Namespace: public ScopeItem {
      public:
        virtual const NodeType nodeType();
        virtual std::shared_ptr<Node> clone();
        virtual std::shared_ptr<Node> deepClone();

        static std::shared_ptr<Namespace> create(std::shared_ptr<Scope> parentScope, std::string name);

        std::shared_ptr<Scope> scope;
        std::vector<std::shared_ptr<Type>> hoistedFunctionalTypes;

        Namespace(std::string name, std::shared_ptr<Scope> parentScope = nullptr);
    };
  };
};

#endif // ALTACORE_DET_NAMESPACE_HPP
