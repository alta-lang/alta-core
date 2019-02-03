#ifndef ALTACORE_AST_ACCESSOR_HPP
#define ALTACORE_AST_ACCESSOR_HPP

#include "expression-node.hpp"
#include "../det/scope-item.hpp"
#include "../det/function.hpp"

namespace AltaCore {
  namespace AST {
    class Accessor: public ExpressionNode {
      public:
        virtual const NodeType nodeType();

        std::shared_ptr<AST::ExpressionNode> target = nullptr;
        std::string query;

        std::vector<std::shared_ptr<DET::ScopeItem>> $items;
        std::shared_ptr<DET::Function> $readAccessor = nullptr;
        std::shared_ptr<DET::Function> $writeAccessor = nullptr;
        std::shared_ptr<DET::ScopeItem> $narrowedTo = nullptr;
        std::shared_ptr<DET::Type> $targetType = nullptr;

        bool accessesNamespace = false;

        Accessor(std::shared_ptr<AST::ExpressionNode> target, std::string query);

        void narrowTo(std::shared_ptr<DET::Type> type);
        virtual void detail(std::shared_ptr<DET::Scope> scope);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_ACCESSOR_HPP
