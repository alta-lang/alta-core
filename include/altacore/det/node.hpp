#ifndef ALTACORE_DET_NODE_HPP
#define ALTACORE_DET_NODE_HPP

#include "../det-shared.hpp"
#include <memory>
#include <string>

namespace AltaCore {
  namespace DET {
    class Node {
      public:
        virtual ~Node() = default;

        std::string id;

        Node();

        virtual const NodeType nodeType();
        std::shared_ptr<Node> clone();
        std::shared_ptr<Node> deepClone();
    };
  };
};

#endif // ALTACORE_DET_NODE_HPP
