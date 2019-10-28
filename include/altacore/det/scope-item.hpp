#ifndef ALTACORE_DET_SCOPE_ITEM_HPP
#define ALTACORE_DET_SCOPE_ITEM_HPP

#include "node.hpp"
#include <string>
#include <vector>

namespace AltaCore {
  namespace AST {
    class Node; // forward declaration
  };
  namespace DET {
    class Scope; // forward declaration

    class ScopeItem: public Node {
      public:
        virtual const NodeType nodeType();
        virtual std::shared_ptr<Node> clone();
        virtual std::shared_ptr<Node> deepClone();

        Visibility visibility = Visibility::Public;
        std::weak_ptr<Scope> parentScope;
        std::string name;
        size_t genericParameterCount = 0;
        size_t moduleIndex = 0;
        size_t itemID = 0;

        std::vector<std::shared_ptr<ScopeItem>> privateHoistedItems;
        std::vector<std::shared_ptr<ScopeItem>> publicHoistedItems;

        bool instantiatedFromSamePackage = false;

        ScopeItem(std::string name, std::shared_ptr<Scope> parentScope = nullptr);

        static std::vector<std::shared_ptr<ScopeItem>> getUnderlyingItems(std::shared_ptr<DH::Node> node);

        virtual std::string toString() const;
        virtual std::vector<std::shared_ptr<ScopeItem>> fullPrivateHoistedItems() const {
          return privateHoistedItems;
        };
    };
  };
};

#endif // ALTACORE_DET_SCOPE_ITEM_HPP
