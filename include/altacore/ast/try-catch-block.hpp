#ifndef ALTACORE_AST_TRY_CATCH_BLOCK_HPP
#define ALTACORE_AST_TRY_CATCH_BLOCK_HPP

#include "statement-node.hpp"
#include "block-node.hpp"
#include "type.hpp"
#include "../det/scope.hpp"
#include <vector>

namespace AltaCore {
  namespace AST {
    class TryCatchBlock: public StatementNode {
      public:
        virtual const NodeType nodeType();

        std::shared_ptr<StatementNode> tryBlock = nullptr;
        std::vector<std::string> catchIDs;
        std::vector<std::pair<std::shared_ptr<Type>, std::shared_ptr<StatementNode>>> catchBlocks;
        std::shared_ptr<StatementNode> catchAllBlock = nullptr;

        TryCatchBlock() {};
        
        ALTACORE_AST_DETAIL(TryCatchBlock);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_TRY_CATCH_BLOCK_HPP
