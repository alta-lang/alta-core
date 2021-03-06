#ifndef ALTACORE_AST_ACCESSOR_HPP
#define ALTACORE_AST_ACCESSOR_HPP

#include "retrieval-node.hpp"
#include "../det/scope-item.hpp"
#include "../det/function.hpp"
#include "type.hpp"

namespace AltaCore {
  namespace AST {
    class Accessor: public RetrievalNode {
      public:
        virtual const NodeType nodeType();

        std::shared_ptr<AST::ExpressionNode> target = nullptr;
        std::vector<std::shared_ptr<Type>> genericArguments;

        Accessor(std::shared_ptr<AST::ExpressionNode> target, std::string query);

        void narrowTo(std::shared_ptr<DH::Accessor> info, std::shared_ptr<DET::Type> type);
        void narrowTo(std::shared_ptr<DH::Accessor> info, size_t i);
        void widen(std::shared_ptr<DH::Accessor> info);
        
        ALTACORE_AST_DETAIL(Accessor);
        ALTACORE_AST_VALIDATE;
    };
  };
};

#endif // ALTACORE_AST_ACCESSOR_HPP
