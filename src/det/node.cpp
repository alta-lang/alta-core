#include "../include/altacore/det/node.hpp"

const AltaCore::DET::NodeType AltaCore::DET::Node::nodeType() {
  return NodeType::Node;
};

AltaCore::DET::Node* AltaCore::DET::Node::clone() {
  return new Node(*this);
};

AltaCore::DET::Node* AltaCore::DET::Node::deepClone() {
  return clone();
};