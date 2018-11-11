#ifndef ALTACORE_AST_NODE_HPP
#define ALTACORE_AST_NODE_HPP

#include "../ast-shared.hpp"
#include "../det/scope.hpp"

namespace AltaCore {
  namespace AST {
    class Node {
      public:
        virtual ~Node() = default;

        virtual const NodeType nodeType();

        virtual void detail(std::shared_ptr<DET::Scope> scope);
    };
  };
};

#endif // ALTACORE_AST_NODE_HPP
