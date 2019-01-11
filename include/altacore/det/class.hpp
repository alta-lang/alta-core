#ifndef ALTACORE_DET_CLASS_HPP
#define ALTACORE_DET_CLASS_HPP

#include "scope-item.hpp"
#include "scope.hpp"
#include <string>

namespace AltaCore {
  namespace DET {
    class Class: public ScopeItem {
      public:
        virtual const NodeType nodeType();
        virtual std::shared_ptr<Node> clone();
        virtual std::shared_ptr<Node> deepClone();

        static std::shared_ptr<Class> create(std::string name, std::shared_ptr<Scope> parentScope);

        std::shared_ptr<Scope> scope = nullptr;
        std::shared_ptr<Function> defaultConstructor = nullptr;
        std::vector<std::shared_ptr<Function>> constructors;
        std::shared_ptr<Function> destructor = nullptr;

        Class(std::string name, std::shared_ptr<Scope> parentScope);
    };
  };
};

#endif // ALTACORE_DET_CLASS_HPP
