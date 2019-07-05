#ifndef ALTACORE_DET_MODULE_HPP
#define ALTACORE_DET_MODULE_HPP

#include "node.hpp"
#include "scope.hpp"
#include <string>
#include "../fs.hpp"
#include "../modules.hpp"
#include "../simple-map.hpp"

namespace AltaCore {
  namespace AST {
    class RootNode; // forward declaration
  };

  namespace DET {
    class Module: public Node {
      public:
        virtual const NodeType nodeType();
        virtual std::shared_ptr<Node> clone();
        virtual std::shared_ptr<Node> deepClone();
        
        static std::shared_ptr<Module> create(std::string name, Filesystem::Path path = Filesystem::Path());
        static std::shared_ptr<Module> create(std::string name, Modules::PackageInfo packageInfo, Filesystem::Path path = Filesystem::Path());

        std::string name;
        Filesystem::Path path;
        std::shared_ptr<Scope> scope;
        std::shared_ptr<Scope> exports;
        std::shared_ptr<Module> parent = nullptr;
        std::vector<std::shared_ptr<Module>> dependencies;
        std::vector<std::shared_ptr<Module>> dependents;
        ALTACORE_MAP<std::string, std::vector<std::shared_ptr<Module>>> genericDependencies;
        std::vector<std::shared_ptr<ScopeItem>> genericsUsed;
        std::weak_ptr<AST::RootNode> ast;
        Modules::PackageInfo packageInfo;
        bool noRuntimeInclude = false;

        std::vector<std::shared_ptr<ScopeItem>> hoistedItems;
        size_t rootItemCount = 0;

        Module();
    };
  };
};

#endif // ALTACORE_DET_MODULE_HPP
