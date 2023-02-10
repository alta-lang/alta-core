#include "../../include/altacore/ast/variable-declaration-statement.hpp"
#include "../../include/altacore/util.hpp"

const AltaCore::AST::NodeType AltaCore::AST::VariableDeclarationStatement::nodeType() {
  return NodeType::VariableDeclarationStatement;
};

ALTACORE_AST_DETAIL_D(VariableDeclarationStatement) {
  ALTACORE_MAKE_DH(VariableDeclarationStatement);

  info->type = type->fullDetail(info->inputScope);

  info->variable = std::make_shared<DET::Variable>(name, info->type->type, position, info->inputScope);
  info->inputScope->items.push_back(info->variable);
  info->variable->isLiteral = std::find(modifiers.begin(), modifiers.end(), "literal") != modifiers.end();
  info->variable->isExport = std::find(modifiers.begin(), modifiers.end(), "export") != modifiers.end();
  if (info->variable->isExport) {
    if (auto mod = Util::getModule(info->inputScope.get()).lock()) {
      mod->exports->items.push_back(info->variable);
    }
  }

  for (auto& attr: attributes) {
    info->attributes.push_back(attr->fullDetail(info->inputScope, shared_from_this(), info));
  }

  return info;
};

ALTACORE_AST_VALIDATE_D(VariableDeclarationStatement) {
  ALTACORE_VS_S(VariableDeclarationStatement);

  if (name.empty()) {
    ALTACORE_VALIDATION_ERROR("empty name for variable declaration");
  }

  if (!type) {
    ALTACORE_VALIDATION_ERROR("no type for variable declaration");
  }

  type->validate(stack, info->type);

  ALTACORE_VS_E;
};
