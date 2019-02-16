#include "../../include/altacore/ast/class-definition-node.hpp"
#include "../../include/altacore/ast/class-special-method-definition-statement.hpp"
#include "../../include/altacore/util.hpp"

const AltaCore::AST::NodeType AltaCore::AST::ClassDefinitionNode::nodeType() {
  return NodeType::ClassDefinitionNode;
};

AltaCore::AST::ClassDefinitionNode::ClassDefinitionNode(std::string _name):
  name(_name)
  {};

ALTACORE_AST_DETAIL_D(ClassDefinitionNode) {
  ALTACORE_MAKE_DH(ClassDefinitionNode);
  info->klass = DET::Class::create(name, scope);
  scope->items.push_back(info->klass);

  info->isLiteral = std::find(modifiers.begin(), modifiers.end(), "literal") != modifiers.end();
  info->isExport = std::find(modifiers.begin(), modifiers.end(), "export") != modifiers.end();

  auto loop = [&](std::vector<std::shared_ptr<ClassStatementNode>>& tgt) -> void {
    for (auto stmt: tgt) {
      auto det = stmt->fullDetail(info->klass->scope);
      info->statements.push_back(det);
      if (stmt->nodeType() == NodeType::ClassSpecialMethodDefinitionStatement) {
        auto special = std::dynamic_pointer_cast<ClassSpecialMethodDefinitionStatement>(stmt);
        auto specialDet = std::dynamic_pointer_cast<DH::ClassSpecialMethodDefinitionStatement>(det);
        if (special->type == SpecialClassMethod::Constructor) {
          info->klass->constructors.push_back(specialDet->method);
        } else {
          ALTACORE_DETAILING_ERROR("destructors aren't supported yet");
        }
      }
    }
  };

  loop(statements);

  if (info->klass->constructors.size() == 0) {
    info->createDefaultConstructor = true;
    info->defaultConstructor = std::make_shared<ClassSpecialMethodDefinitionStatement>(Visibility::Public, SpecialClassMethod::Constructor);
    info->defaultConstructor->body = std::make_shared<BlockNode>();
    info->defaultConstructorDetail = info->defaultConstructor->fullDetail(info->klass->scope);
    info->klass->constructors.push_back(info->defaultConstructorDetail->method);
  }

  if (info->isExport) {
    if (auto mod = Util::getModule(scope.get()).lock()) {
      mod->exports->items.push_back(info->klass);
    }
  }
  return info;
};

ALTACORE_AST_VALIDATE_D(ClassDefinitionNode) {
  ALTACORE_VS_S(ClassDefinitionNode);
  for (auto& mod: modifiers) {
    if (mod.empty()) ALTACORE_VALIDATION_ERROR("empty modifier for class defintion");
  }
  if (name.empty()) ALTACORE_VALIDATION_ERROR("empty name for class definition");
  for (size_t i = 0; i < statements.size(); i++) {
    auto& stmt = statements[i];
    auto& stmtDet = info->statements[i];
    if (!stmt) ALTACORE_VALIDATION_ERROR("empty class statement in class definition");
    stmt->validate(stack, stmtDet);
  }
  if (!info->klass) ALTACORE_VALIDATION_ERROR("class defintion not detailed properly, no DET class attached");
  ALTACORE_VS_E;
};
