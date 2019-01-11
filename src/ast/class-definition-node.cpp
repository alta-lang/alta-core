#include "../../include/altacore/ast/class-definition-node.hpp"

const AltaCore::AST::NodeType AltaCore::AST::ClassDefinitionNode::nodeType() {
  return NodeType::ClassDefinitionNode;
};

AltaCore::AST::ClassDefinitionNode::ClassDefinitionNode(std::string _name):
  name(_name)
  {};

void AltaCore::AST::ClassDefinitionNode::detail(std::shared_ptr<AltaCore::DET::Scope> scope) {
  $klass = DET::Class::create(name, scope);
  scope->items.push_back($klass);

  auto loop = [&](std::vector<std::shared_ptr<ClassStatementNode>>& tgt) -> void {
    for (auto stmt: tgt) {
      if (stmt->nodeType() == NodeType::ClassSpecialMethodDefinitionStatement) {
        throw std::runtime_error("special class methods aren't supported yet");
      } else {
        stmt->detail($klass->scope);
      }
    }
  };

  loop(statements);
};
