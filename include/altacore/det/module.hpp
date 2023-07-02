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
    class Module;
    class Namespace;
    class Class;
    class Variable;

    struct InternalPackage {
      public:
        std::shared_ptr<Module> module = nullptr;
        std::shared_ptr<Module> coroutinesModule = nullptr;

        std::shared_ptr<Namespace> coroutinesNamespace = nullptr;
        std::shared_ptr<Class> schedulerClass = nullptr;
        std::shared_ptr<Class> resultClass = nullptr;

        std::shared_ptr<Variable> schedulerVariable = nullptr;
    };

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
        std::vector<std::shared_ptr<Module>> dependencies;
        std::vector<std::shared_ptr<Module>> dependents;
        ALTACORE_MAP<std::string, std::vector<std::shared_ptr<Module>>> genericDependencies;
        std::vector<std::shared_ptr<ScopeItem>> genericsUsed;
        std::weak_ptr<AST::RootNode> ast;
        Modules::PackageInfo packageInfo;
        bool noRuntimeInclude = false;
        InternalPackage internal;
        // NOTE: this CANNOT be reliably used to determine what Module imported this Module,
        //       it only refers to the one the *initially* imported it, because after the initial import,
        //       the module remains in the cache. this is only useful to us right now for some hacks that we have to do
        //       for the `_internal` package in root-node.cpp
        std::shared_ptr<Module> parentModule = nullptr;

        std::vector<std::shared_ptr<ScopeItem>> hoistedItems;
        size_t rootItemCount = 0;

        Module();

        virtual std::string toString() const;
    };
  };
};

#endif // ALTACORE_DET_MODULE_HPP
