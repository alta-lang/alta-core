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
        std::shared_ptr<Type> type;

        Parameter(std::string name, std::shared_ptr<Type> type);

        virtual void detail(std::shared_ptr<DET::Scope> scope);
    };
  };
};

#endif // ALTACORE_AST_PARAMETER_HPP
