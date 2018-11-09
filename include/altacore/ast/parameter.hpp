#ifndef ALTACORE_AST_PARAMETER_HPP
#define ALTACORE_AST_PARAMETER_HPP

#include "node.hpp"
#include "parameter.hpp"
#include "type.hpp"
#include "../det/scope.hpp"
#include <string>
#include <vector>

namespace AltaCore {
  namespace AST {
    class Parameter: public Node {
      public:
        virtual const NodeType nodeType();

        std::string name;
        Type* type;

        Parameter(std::string name, Type* type);

        virtual void detail(DET::Scope* scope);
    };
  };
};

#endif // ALTACORE_AST_PARAMETER_HPP