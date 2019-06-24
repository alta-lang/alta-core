#ifndef ALTACORE_AST_CODE_LITERAL_NODE_HPP
#define ALTACORE_AST_CODE_LITERAL_NODE_HPP

#include "statement-node.hpp"

namespace AltaCore {
  namespace AST {
    class CodeLiteralNode: public StatementNode {
      public:
        virtual const NodeType nodeType();

        std::string raw;

        CodeLiteralNode(std::string raw);

        ALTACORE_AST_AUTO_DETAIL(CodeLiteralNode);
    };
  };
};

#endif // ALTACORE_AST_CODE_LITERAL_NODE_HPP
