#include "../../include/altacore/ast/node.hpp"

const AltaCore::AST::NodeType AltaCore::AST::Node::nodeType() {
  return NodeType::Node;
};

void AltaCore::AST::Node::detail(std::shared_ptr<AltaCore::DET::Scope> scope) {};
