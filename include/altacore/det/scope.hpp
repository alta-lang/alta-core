#ifndef ALTACORE_DET_SCOPE_HPP
#define ALTACORE_DET_SCOPE_HPP

#include "node.hpp"
#include "scope-item.hpp"
#include "type.hpp"
#include <vector>
#include <cinttypes>
#include <string>
#include <unordered_set>
#include "../errors.hpp"

namespace AltaCore {
  namespace DET {
    class Class; // forward declaration

    class Scope: public Node, public std::enable_shared_from_this<Scope> {
      public:
        virtual const NodeType nodeType();
        virtual std::shared_ptr<Node> clone();
        virtual std::shared_ptr<Node> deepClone();

        std::weak_ptr<Scope> parent;
        std::weak_ptr<Module> parentModule;
        std::weak_ptr<Function> parentFunction;
        std::weak_ptr<Namespace> parentNamespace;
        std::weak_ptr<Class> parentClass;

        std::vector<std::shared_ptr<ScopeItem>> items;
        std::vector<std::shared_ptr<Scope>> childScopes;
        size_t relativeID = 0;
        size_t nextChildID = 0;
        size_t nextItemID = 0;
        bool noRuntime = false;
        bool isLoopScope = false;

        bool isTry = false;
        std::unordered_set<std::shared_ptr<Type>, TypePointerHash, TypePointerComparator> typesThrown;
        AltaCore::Errors::Position position;

        Scope(AltaCore::Errors::Position position);
        Scope(std::shared_ptr<Scope> parent, AltaCore::Errors::Position position);
        Scope(std::shared_ptr<Module> parentModule);
        Scope(std::shared_ptr<Function> parentFunction);
        Scope(std::shared_ptr<Namespace> parentNamespace);
        Scope(std::shared_ptr<Class> parentClass);

        static std::shared_ptr<Scope> makeWithParentScope(std::shared_ptr<Scope> parent, AltaCore::Errors::Position position);

        std::vector<std::shared_ptr<ScopeItem>> findAll(std::string name, std::vector<std::shared_ptr<Type>> excludeTypes = {}, bool searchParents = true, std::shared_ptr<Scope> originScope = nullptr);
        void hoist(std::shared_ptr<ScopeItem> item);
        void unhoist(std::shared_ptr<ScopeItem> item);

        bool hasParent(std::shared_ptr<Scope> parent) const;

        static std::shared_ptr<Scope> getMemberScope(std::shared_ptr<ScopeItem> item);

        bool canSee(std::shared_ptr<ScopeItem> item) const;

        std::weak_ptr<Scope> findTry();
        void addPossibleError(std::shared_ptr<Type> errorType);

        bool contains(std::shared_ptr<ScopeItem> item);
        std::shared_ptr<DET::Function> findParentLambda();
        std::shared_ptr<DET::Class> findParentCaptureClass();
        std::shared_ptr<Scope> findClosestParentScope();

        virtual std::string toString() const;
    };
  };
};

#endif //
