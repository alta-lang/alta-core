#include "../../include/altacore/ast/variable-definition-expression.hpp"
#include "../../include/altacore/util.hpp"

const AltaCore::AST::NodeType AltaCore::AST::VariableDefinitionExpression::nodeType() {
  return NodeType::VariableDefinitionExpression;
};

AltaCore::AST::VariableDefinitionExpression::VariableDefinitionExpression(
  std::string _name,
  std::shared_ptr<AltaCore::AST::Type> _type,
  std::shared_ptr<AltaCore::AST::ExpressionNode> _initializationExpression
):
  name(_name),
  type(_type),
  initializationExpression(_initializationExpression)
  {};

ALTACORE_AST_DETAIL_NO_BODY_OPT_D(VariableDefinitionExpression) {
  ALTACORE_MAKE_DH(VariableDefinitionExpression);
  return detail(info, noBody);
};

ALTACORE_AST_VALIDATE_D(VariableDefinitionExpression) {
  ALTACORE_VS_S(VariableDefinitionExpression);
  if (name.empty()) ALTACORE_VALIDATION_ERROR("empty name for variable definition");
  if (!type) ALTACORE_VALIDATION_ERROR("empty type for variable definition");
  type->validate(stack, info->type);
  if (initializationExpression) {
    initializationExpression->validate(stack, info->initializationExpression);
  }
  for (auto& mod: modifiers) {
    if (mod.empty()) ALTACORE_VALIDATION_ERROR("empty modifer for variable definition");
  }
  ALTACORE_VS_E;
};

ALTACORE_AST_INFO_DETAIL_D(VariableDefinitionExpression) {
  ALTACORE_CAST_DH(VariableDefinitionExpression);

  if (!info->type) {
    info->type = type->fullDetail(info->inputScope);
  }

  if (!info->variable) {
    info->variable = std::make_shared<DET::Variable>(name, info->type->type, info->inputScope);
    info->inputScope->items.push_back(info->variable);
    info->variable->isLiteral = std::find(modifiers.begin(), modifiers.end(), "literal") != modifiers.end();
    info->variable->isExport = std::find(modifiers.begin(), modifiers.end(), "export") != modifiers.end();
    if (info->variable->isExport) {
      if (auto mod = Util::getModule(info->inputScope.get()).lock()) {
        mod->exports->items.push_back(info->variable);
      }
    }
  }

  if (initializationExpression != nullptr && !noBody && !info->initializationExpression) {
    info->initializationExpression = initializationExpression->fullDetail(info->inputScope);
  }

  return info;
};
