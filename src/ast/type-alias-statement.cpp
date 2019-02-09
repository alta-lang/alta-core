#include "../../include/altacore/ast/type-alias-statement.hpp"
#include "../../include/altacore/util.hpp"

const AltaCore::AST::NodeType AltaCore::AST::TypeAliasStatement::nodeType() {
  return NodeType::TypeAliasStatement;
};

void AltaCore::AST::TypeAliasStatement::detail(std::shared_ptr<AltaCore::DET::Scope> scope) {
  type->detail(scope);
  type->$type->name = name;
  scope->items.push_back(type->$type);

  for (auto& mod: modifiers) {
    if (mod == "export") {
      isExport = true;
    }
  }

  if (isExport) {
    if (auto mod = Util::getModule(scope.get()).lock()) {
      mod->exports->items.push_back(type->$type);
    }
  }
};

ALTACORE_AST_VALIDATE_D(TypeAliasStatement) {
  ALTACORE_VS_S;
  if (name.empty()) ALTACORE_VALIDATION_ERROR("empty name for type alias");
  if (!type) ALTACORE_VALIDATION_ERROR("empty type for type alias");
  type->validate(stack);
  ALTACORE_VS_E;
};
