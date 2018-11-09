#include "../include/altacore/ast/node.hpp"

const AltaCore::AST::NodeType AltaCore::AST::Node::nodeType() {
  return NodeType::Node;
};

void AltaCore::AST::Node::detail(AltaCore::DET::Scope* scope) {};