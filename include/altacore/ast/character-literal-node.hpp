#ifndef ALTACORE_AST_CHARACTER_LITERAL_NODE_HPP
#define ALTACORE_AST_CHARACTER_LITERAL_NODE_HPP

#include "literal-node.hpp"

namespace AltaCore {
  namespace AST {
    class CharacterLiteralNode: public LiteralNode {
      public:
        virtual const NodeType nodeType();

        char value;
        bool escaped;

        CharacterLiteralNode(char value, bool escaped = false);
    };
  };
};

#endif // ALTACORE_AST_CHARACTER_LITERAL_NODE_HPP
