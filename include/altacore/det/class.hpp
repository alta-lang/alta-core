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

    class Class: public ScopeItem, public std::enable_shared_from_this<Class> {
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
        bool isBitfield = false;

        std::shared_ptr<Scope> scope = nullptr;
        std::shared_ptr<Function> defaultConstructor = nullptr;
        std::vector<std::shared_ptr<Function>> constructors;
        std::shared_ptr<Function> destructor = nullptr;
        std::vector<std::shared_ptr<Class>> parents;
        std::shared_ptr<Function> copyConstructor = nullptr;
        std::vector<std::shared_ptr<Variable>> members;
        std::vector<std::shared_ptr<Function>> fromCasts;
        std::vector<std::shared_ptr<Function>> toCasts;
        std::vector<std::shared_ptr<Function>> operators;

        std::vector<std::shared_ptr<Type>> genericArguments;

        std::weak_ptr<Type> underlyingBitfieldType;

        std::weak_ptr<AST::ClassDefinitionNode> ast;
        std::weak_ptr<DetailHandles::ClassDefinitionNode> info;

        Class(std::string name, std::shared_ptr<Scope> parentScope, std::vector<std::shared_ptr<Class>> parents = {});

        bool hasParent(std::shared_ptr<Class> parent) const;
        std::shared_ptr<Class> instantiateGeneric(std::vector<std::shared_ptr<Type>> genericArguments);

        std::shared_ptr<Function> findFromCast(const Type& target);
        std::shared_ptr<Function> findToCast(const Type& target);

        std::shared_ptr<Function> findOperator(const Shared::ClassOperatorType type, const Shared::ClassOperatorOrientation orient, std::shared_ptr<Type> argType = nullptr) const;

        std::vector<std::shared_ptr<Function>> findAllVirtualFunctions();

        virtual std::string toString() const;
    };
  };
};

#endif // ALTACORE_DET_CLASS_HPP
