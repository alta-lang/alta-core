#ifndef ALTACORE_DET_SCOPE_HPP
#define ALTACORE_DET_SCOPE_HPP

#include "node.hpp"
#include "scope-item.hpp"
#include <vector>
#include <cinttypes>
#include <string>

namespace AltaCore {
  namespace DET {
    class Scope: public Node {
      public:
        virtual const NodeType nodeType();
        virtual Scope* clone();
        virtual Scope* deepClone();

        Scope* parent = nullptr;
        Module* parentModule = nullptr;
        Function* parentFunction = nullptr;

        std::vector<ScopeItem*> items;
        size_t relativeID = 0;
        size_t nextChildID = 0;

        Scope();
        Scope(Scope* parent);
        Scope(Module* parentModule);
        Scope(Function* parentFunction);

        std::vector<ScopeItem*> findAll(std::string name);
    };
  };
};

#endif //