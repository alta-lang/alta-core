#include "../../include/altacore/ast/class-operator-definition-statement.hpp"
#include "../../include/altacore/det/type.hpp"

const AltaCore::AST::NodeType AltaCore::AST::ClassOperatorDefinitionStatement::nodeType() {
  return NodeType::ClassOperatorDefinitionStatement;
};

AltaCore::AST::ClassOperatorDefinitionStatement::ClassOperatorDefinitionStatement(AltaCore::AST::Visibility _visibilityModifier):
  visibilityModifier(_visibilityModifier)
  {};

ALTACORE_AST_DETAIL_NO_BODY_OPT_D(ClassOperatorDefinitionStatement) {
  ALTACORE_MAKE_DH(ClassOperatorDefinitionStatement);
  return detail(info, noBody);
};

ALTACORE_AST_VALIDATE_D(ClassOperatorDefinitionStatement) {
  ALTACORE_VS_S(ClassOperatorDefinitionStatement);
  returnType->validate(stack, info->returnType);
  if (argumentType)
    argumentType->validate(stack, info->argumentType);
  block->validate(stack, info->block);
  ALTACORE_VS_E;
};

ALTACORE_AST_INFO_DETAIL_D(ClassOperatorDefinitionStatement) {
  ALTACORE_CAST_DH(ClassOperatorDefinitionStatement);

  if (!info->method) {
    info->method = DET::Function::create(info->inputScope, std::string("@operator@") + ClassOperatorType_names[(size_t)type] + "@" + Shared::ClassOperatorOrientation_names[(size_t)orientation], {}, nullptr);
    info->method->visibility = visibilityModifier;
    info->method->isMethod = true;
    info->method->isOperator = true;
    info->method->operatorType = type;
    info->method->orientation = orientation;
    auto klass = info->inputScope->parentClass.lock();
    info->method->parentClassType = std::make_shared<DET::Type>(klass, std::vector<uint8_t> { (uint8_t)TypeModifierFlag::Reference });
  }

  std::vector<std::tuple<std::string, std::shared_ptr<DET::Type>, bool, std::string>> params;

  if (!info->argumentType && argumentType) {
    info->argumentType = argumentType->fullDetail(info->method->scope);
  }

  if (!info->returnType) {
    info->returnType = returnType->fullDetail(info->method->scope);
  }

  if (!info->method->returnType) {
    std::vector<std::tuple<std::string, std::shared_ptr<DET::Type>, bool, std::string>> params;
    bool hasArg = type == ClassOperatorType::Index || orientation != ClassOperatorOrientation::Unary;
    if (hasArg) {
      params.push_back(std::make_tuple(orientation == ClassOperatorOrientation::Left ? "$right" : "$left", info->argumentType->type, false, "N/A"));
    }
    info->method->recreate(params, info->returnType->type);
    if (hasArg) {
      auto inputAlias = std::make_shared<DET::Alias>("$", info->method->parameterVariables.front(), info->method->scope);
      info->method->scope->items.push_back(inputAlias);
    }
  }

  if (!noBody && !info->block) {
    info->block = block->fullDetail(info->method->scope);
  }

  return info;
};
