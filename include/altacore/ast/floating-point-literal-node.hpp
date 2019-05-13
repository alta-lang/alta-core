#ifndef ALTACORE_AST_FLOATING_POINT_LITERAL_NODE_HPP
#define ALTACORE_AST_FLOATING_POINT_LITERAL_NODE_HPP

#include "literal-node.hpp"

namespace AltaCore {
  namespace AST {
    class FloatingPointLiteralNode: public LiteralNode {
      public:
        virtual const NodeType nodeType();

        FloatingPointLiteralNode(std::string raw);

        ALTACORE_AST_AUTO_DETAIL(FloatingPointLiteralNode);
    };
  };
};

#endif // ALTACORE_AST_FLOATING_POINT_LITERAL_NODE_HPP
