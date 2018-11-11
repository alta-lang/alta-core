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
        virtual std::shared_ptr<Node> clone();
        virtual std::shared_ptr<Node> deepClone();
        
        static std::shared_ptr<Module> create(std::string name, Filesystem::Path path = Filesystem::Path());

        std::string name;
        Filesystem::Path path;
        std::shared_ptr<Scope> scope;
        std::shared_ptr<Module> parent = nullptr;
        std::vector<std::shared_ptr<Module>> dependencies;
        std::vector<std::shared_ptr<Module>> dependents;

        Module();
    };
  };
};

#endif // ALTACORE_DET_MODULE_HPP
