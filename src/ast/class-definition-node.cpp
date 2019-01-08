#include "../../include/altacore/ast/class-definition-node.hpp"

const AltaCore::AST::NodeType AltaCore::AST::ClassDefinitionNode::nodeType() {
  return NodeType::ClassDefinitionNode;
};

AltaCore::AST::ClassDefinitionNode::ClassDefinitionNode(std::string _name):
  name(_name)
  {};

void AltaCore::AST::ClassDefinitionNode::detail(std::shared_ptr<AltaCore::DET::Scope> scope) {

};
