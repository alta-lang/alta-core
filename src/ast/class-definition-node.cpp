#include "../../include/altacore/ast/class-definition-node.hpp"
#include "../../include/altacore/ast/class-special-method-definition-statement.hpp"
#include "../../include/altacore/util.hpp"

const AltaCore::AST::NodeType AltaCore::AST::ClassDefinitionNode::nodeType() {
  return NodeType::ClassDefinitionNode;
};

AltaCore::AST::ClassDefinitionNode::ClassDefinitionNode(std::string _name):
  name(_name)
  {};

void AltaCore::AST::ClassDefinitionNode::detail(std::shared_ptr<AltaCore::DET::Scope> scope) {
  $klass = DET::Class::create(name, scope);
  scope->items.push_back($klass);

  isLiteral = std::find(modifiers.begin(), modifiers.end(), "literal") != modifiers.end();
  isExport = std::find(modifiers.begin(), modifiers.end(), "export") != modifiers.end();

  auto loop = [&](std::vector<std::shared_ptr<ClassStatementNode>>& tgt) -> void {
    for (auto stmt: tgt) {
      stmt->detail($klass->scope);
      if (stmt->nodeType() == NodeType::ClassSpecialMethodDefinitionStatement) {
        auto special = std::dynamic_pointer_cast<ClassSpecialMethodDefinitionStatement>(stmt);
        if (special->type == SpecialClassMethod::Constructor) {
          $klass->constructors.push_back(special->$method);
        } else {
          throw std::runtime_error("destructors aren't supported yet");
        }
      }
    }
  };

  loop(statements);

  if ($klass->constructors.size() == 0) {
    $createDefaultConstructor = true;
    $defaultConstructor = std::make_shared<ClassSpecialMethodDefinitionStatement>(Visibility::Public, SpecialClassMethod::Constructor);
    $defaultConstructor->body = std::make_shared<BlockNode>();
    $defaultConstructor->detail($klass->scope);
    $klass->constructors.push_back($defaultConstructor->$method);
  }

  if (isExport) {
    if (auto mod = Util::getModule(scope.get()).lock()) {
      mod->exports->items.push_back($klass);
    }
  }
};

ALTACORE_AST_VALIDATE_D(ClassDefinitionNode) {
  
};