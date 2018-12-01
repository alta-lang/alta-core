#ifndef ALTACORE_AST_LITERAL_NODE_HPP
#define ALTACORE_AST_LITERAL_NODE_HPP

#include "expression-node.hpp"
#include <string>

namespace AltaCore {
  namespace AST {
    class LiteralNode: public ExpressionNode {
      public:
        virtual const NodeType nodeType();

        std::string raw;

        LiteralNode(std::string raw);
    };
  };
};

#endif // ALTACORE_AST_LITERAL_NODE_HPP
