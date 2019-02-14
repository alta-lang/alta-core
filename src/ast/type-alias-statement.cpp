#include "../../include/altacore/ast/type-alias-statement.hpp"
#include "../../include/altacore/util.hpp"

const AltaCore::AST::NodeType AltaCore::AST::TypeAliasStatement::nodeType() {
  return NodeType::TypeAliasStatement;
};

ALTACORE_AST_DETAIL_D(TypeAliasStatement) {
  ALTACORE_MAKE_DH(TypeAliasStatement);
  info->type = type->fullDetail(scope);
  info->type->type->name = name;
  scope->items.push_back(info->type->type);

  for (auto& mod: modifiers) {
    if (mod == "export") {
      info->isExport = true;
    }
  }

  if (info->isExport) {
    if (auto mod = Util::getModule(scope.get()).lock()) {
      mod->exports->items.push_back(info->type->type);
    }
  }
  return info;
};

ALTACORE_AST_VALIDATE_D(TypeAliasStatement) {
  ALTACORE_VS_S(TypeAliasStatement);
  if (name.empty()) ALTACORE_VALIDATION_ERROR("empty name for type alias");
  if (!type) ALTACORE_VALIDATION_ERROR("empty type for type alias");
  type->validate(stack, info->type);
  ALTACORE_VS_E;
};
