#ifndef ALTACORE_AST_BLOCK_NODE_HPP
#define ALTACORE_AST_BLOCK_NODE_HPP

#include "statement-node.hpp"
#include "../det/scope.hpp"
#include <vector>

namespace AltaCore {
  namespace AST {
    class BlockNode: public StatementNode {
      public:
        virtual const NodeType nodeType();

        std::vector<StatementNode*> statements;

        BlockNode();
        BlockNode(std::vector<StatementNode*> statements);
        
        virtual void detail(DET::Scope* scope);
    };
  };
};

#endif // ALTACORE_AST_BLOCK_NODE_HPP