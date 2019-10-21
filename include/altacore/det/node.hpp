#ifndef ALTACORE_DET_NODE_HPP
#define ALTACORE_DET_NODE_HPP

#include "../det-shared.hpp"
#include <memory>
#include <string>
#include <stdexcept>

namespace AltaCore {
  namespace DetailHandles {
    class Node;
    class ExpressionNode;
  };
  namespace DH = DetailHandles;
  namespace DET {
    class Node {
      public:
        virtual ~Node() = default;

        std::string id;

        Node();

        virtual const NodeType nodeType();
        std::shared_ptr<Node> clone();
        std::shared_ptr<Node> deepClone();

        virtual std::string toString() const;
    };
  };
};

#endif // ALTACORE_DET_NODE_HPP
