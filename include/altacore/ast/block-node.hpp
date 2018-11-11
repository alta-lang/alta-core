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

        std::vector<std::shared_ptr<StatementNode>> statements;

        BlockNode();
        BlockNode(std::vector<std::shared_ptr<StatementNode>> statements);
        
        virtual void detail(std::shared_ptr<DET::Scope> scope);
    };
  };
};

#endif // ALTACORE_AST_BLOCK_NODE_HPP
