#ifndef ALTACORE_AST_EXPRESSION_NODE_HPP
#define ALTACORE_AST_EXPRESSION_NODE_HPP

#include "node.hpp"
#include "attribute-node.hpp"

// multiply-inheritable shared_from_this
// adapted from https://stackoverflow.com/a/16083526
class MultiplyInheritableEnableSharedFromThis: public std::enable_shared_from_this<MultiplyInheritableEnableSharedFromThis> {
  public:
    virtual ~MultiplyInheritableEnableSharedFromThis() {}
};

template <class T>
class inheritable_enable_shared_from_this: virtual public MultiplyInheritableEnableSharedFromThis {
  public:
    std::shared_ptr<T> shared_from_this() {
      return std::dynamic_pointer_cast<T>(MultiplyInheritableEnableSharedFromThis::shared_from_this());
    }

    template <class Down>
    std::shared_ptr<Down> downcasted_shared_from_this() {
      return std::dynamic_pointer_cast<Down>(MultiplyInheritableEnableSharedFromThis::shared_from_this());
    }
};

namespace AltaCore {
  namespace AST {
    class ExpressionNode: public Node, public inheritable_enable_shared_from_this<ExpressionNode> {
      public:
        virtual const NodeType nodeType();

        std::vector<std::shared_ptr<AST::AttributeNode>> attributes;

        ALTACORE_AST_AUTO_DETAIL(ExpressionNode);

        void detailAttributes(std::shared_ptr<DH::ExpressionNode> info);
    };
  };
};

#endif // ALTACORE_AST_EXPRESSION_NODE_HPP
