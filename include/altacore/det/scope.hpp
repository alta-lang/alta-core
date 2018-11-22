#ifndef ALTACORE_DET_SCOPE_HPP
#define ALTACORE_DET_SCOPE_HPP

#include "node.hpp"
#include "scope-item.hpp"
#include <vector>
#include <cinttypes>
#include <string>

namespace AltaCore {
  namespace DET {
    class Type; // forward declaration

    class Scope: public Node, public std::enable_shared_from_this<Scope> {
      public:
        virtual const NodeType nodeType();
        virtual std::shared_ptr<Node> clone();
        virtual std::shared_ptr<Node> deepClone();

        std::weak_ptr<Scope> parent;
        std::weak_ptr<Module> parentModule;
        std::weak_ptr<Function> parentFunction;

        std::vector<std::shared_ptr<ScopeItem>> items;
        size_t relativeID = 0;
        size_t nextChildID = 0;

        Scope();
        Scope(std::shared_ptr<Scope> parent);
        Scope(std::shared_ptr<Module> parentModule);
        Scope(std::shared_ptr<Function> parentFunction);

        std::vector<std::shared_ptr<ScopeItem>> findAll(std::string name, std::vector<std::shared_ptr<Type>> excludeTypes = {});
        void hoist(std::shared_ptr<Type> whatToHoist);
    };
  };
};

#endif //
