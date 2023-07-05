#ifndef ALTACORE_DET_NAMESPACE_HPP
#define ALTACORE_DET_NAMESPACE_HPP

#include "scope.hpp"
#include "type.hpp"
#include <string>

namespace AltaCore {
  namespace DET {
    class Function;

    class Namespace: public ScopeItem {
      public:
        virtual const NodeType nodeType();
        virtual std::shared_ptr<Node> clone();
        virtual std::shared_ptr<Node> deepClone();

        static std::shared_ptr<Namespace> create(std::shared_ptr<Scope> parentScope, std::string name, AltaCore::Errors::Position position);

        std::shared_ptr<Scope> scope;
        std::vector<std::shared_ptr<Type>> hoistedFunctionalTypes;
        std::shared_ptr<Type> underlyingEnumerationType = nullptr;
        std::shared_ptr<Function> enumerationLookupFunction = nullptr;
        std::shared_ptr<Function> enumerationReverseLookupFunction = nullptr;

        Namespace(std::string name, AltaCore::Errors::Position position, std::shared_ptr<Scope> parentScope = nullptr);
        virtual std::vector<std::shared_ptr<ScopeItem>> fullPrivateHoistedItems() const;
    };
  };
};

#endif // ALTACORE_DET_NAMESPACE_HPP
