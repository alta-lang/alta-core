#ifndef ALTACORE_AST_CODE_LITERAL_NODE_HPP
#define ALTACORE_AST_CODE_LITERAL_NODE_HPP

#include "statement-node.hpp"
#include "attribute-node.hpp"

namespace AltaCore {
  namespace AST {
    class CodeLiteralNode: public StatementNode, public std::enable_shared_from_this<CodeLiteralNode> {
      public:
        virtual const NodeType nodeType();

        std::string raw;
        std::vector<std::shared_ptr<AttributeNode>> attributes;

        CodeLiteralNode(std::string raw);

        ALTACORE_AST_DETAIL(CodeLiteralNode);
    };
  };
};

#endif // ALTACORE_AST_CODE_LITERAL_NODE_HPP
