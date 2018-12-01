#ifndef ALTACORE_AST_INTEGER_LITERAL_NODE_HPP
#define ALTACORE_AST_INTEGER_LITERAL_NODE_HPP

#include "literal-node.hpp"

namespace AltaCore {
  namespace AST {
    class IntegerLiteralNode: public LiteralNode {
      public:
        virtual const NodeType nodeType();

        IntegerLiteralNode(std::string raw);
    };
  };
};

#endif // ALTACORE_AST_INTEGER_LITERAL_NODE_HPP
