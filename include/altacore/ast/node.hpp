#ifndef ALTACORE_AST_NODE_HPP
#define ALTACORE_AST_NODE_HPP

#include "../ast-shared.hpp"
#include "../det/scope.hpp"

namespace AltaCore {
  namespace AST {
    class Position {
      public:
        size_t line = 0;
        size_t column = 0;

        Position(size_t line = 0, size_t column = 0);
    };
    class Node {
      public:
        virtual ~Node() = default;

        std::string id;
        Position position;

        Node();

        virtual const NodeType nodeType();

        virtual void detail(std::shared_ptr<DET::Scope> scope);
    };
  };
};

#endif // ALTACORE_AST_NODE_HPP
