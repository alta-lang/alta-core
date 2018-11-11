#include "../../include/altacore/det/node.hpp"
#include <memory>

const AltaCore::DET::NodeType AltaCore::DET::Node::nodeType() {
  return NodeType::Node;
};

std::shared_ptr<AltaCore::DET::Node> AltaCore::DET::Node::clone() {
  return std::make_shared<Node>(*this);
};
std::shared_ptr<AltaCore::DET::Node> AltaCore::DET::Node::deepClone() {
  return clone();
};
