#ifndef ALTACORE_AST_NODE_HPP
#define ALTACORE_AST_NODE_HPP

#include "../ast-shared.hpp"
#include "../det/scope.hpp"

namespace AltaCore {
  namespace AST {
    class Node {
      public:
        virtual const NodeType nodeType();

        virtual void detail(DET::Scope* scope);
    };
  };
};

#endif // ALTACORE_AST_NODE_HPP