#ifndef ALTACORE_DET_NODE_HPP
#define ALTACORE_DET_NODE_HPP

#include "../det-shared.hpp"

namespace AltaCore {
  namespace DET {
    class Node {
      public:
        virtual const NodeType nodeType();
        virtual Node* clone();
        virtual Node* deepClone();
    };
  };
};

#endif // ALTACORE_DET_NODE_HPP
