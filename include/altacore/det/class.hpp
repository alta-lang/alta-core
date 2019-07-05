#ifndef ALTACORE_DET_CLASS_HPP
#define ALTACORE_DET_CLASS_HPP

#include "scope-item.hpp"
#include <string>

namespace AltaCore {
  namespace AST {
    class ClassDefinitionNode;
  };
  namespace DetailHandles {
    class ClassDefinitionNode;
  };
  namespace DET {
    class Variable;
    class Scope;
    class Type;

    class Class: public ScopeItem {
      public:
        virtual const NodeType nodeType();
        virtual std::shared_ptr<Node> clone();
        virtual std::shared_ptr<Node> deepClone();

        static std::shared_ptr<Class> create(std::string name, std::shared_ptr<Scope> parentScope, std::vector<std::shared_ptr<Class>> parents = {}, bool isStructure = false);

        bool isStructure = false;
        bool isExternal = false;
        bool isTyped = true;
        bool isLiteral = false;
        bool isExport = false;

        std::shared_ptr<Scope> scope = nullptr;
        std::shared_ptr<Function> defaultConstructor = nullptr;
        std::vector<std::shared_ptr<Function>> constructors;
        std::shared_ptr<Function> destructor = nullptr;
        std::vector<std::shared_ptr<Class>> parents;
        std::shared_ptr<Function> copyConstructor = nullptr;
        std::vector<std::shared_ptr<Variable>> itemsToDestroy;
        std::vector<std::shared_ptr<Variable>> itemsToCopy;

        std::vector<std::shared_ptr<Type>> genericArguments;

        std::weak_ptr<AST::ClassDefinitionNode> ast;
        std::weak_ptr<DetailHandles::ClassDefinitionNode> info;

        Class(std::string name, std::shared_ptr<Scope> parentScope, std::vector<std::shared_ptr<Class>> parents = {});

        bool hasParent(std::shared_ptr<Class> parent) const;
        std::shared_ptr<Class> instantiateGeneric(std::vector<std::shared_ptr<Type>> genericArguments);
    };
  };
};

#endif // ALTACORE_DET_CLASS_HPP
