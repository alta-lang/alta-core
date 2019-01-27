#include "../../include/altacore/ast/node.hpp"
#include <sstream>
#include <crossguid/guid.hpp>

const AltaCore::AST::NodeType AltaCore::AST::Node::nodeType() {
  return NodeType::Node;
};

void AltaCore::AST::Node::detail(std::shared_ptr<AltaCore::DET::Scope> scope) {};

AltaCore::AST::Node::Node() {
  std::stringstream uuidStream;
  uuidStream << xg::newGuid();
  id = uuidStream.str();
};

ALTACORE_AST_VALIDATE_D(Node) {
  ALTACORE_VS_S;
  // by default, nodes are valid, so don't throw any errors
  ALTACORE_VS_E;
};
