#ifndef ALTACORE_DET_MODULE_HPP
#define ALTACORE_DET_MODULE_HPP

#include "node.hpp"
#include "scope.hpp"
#include <string>
#include "../fs.hpp"

namespace AltaCore {
  namespace DET {
    class Module: public Node {
      public:
        virtual const NodeType nodeType();
        virtual Module* clone();
        virtual Module* deepClone();

        std::string name;
        Filesystem::Path path;
        Scope* scope;
        Module* parent = nullptr;
        std::vector<Module*> dependencies;
        std::vector<Module*> dependents;

        Module(std::string name, Filesystem::Path path = Filesystem::Path());
    };
  };
};

#endif // ALTACORE_DET_MODULE_HPP