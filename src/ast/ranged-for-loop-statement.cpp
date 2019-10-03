#include "../../include/altacore/ast/ranged-for-loop-statement.hpp"

const AltaCore::AST::NodeType AltaCore::AST::RangedForLoopStatement::nodeType() {
  return NodeType::RangedForLoopStatement;
};

ALTACORE_AST_DETAIL_D(RangedForLoopStatement) {
  ALTACORE_MAKE_DH(RangedForLoopStatement);
  info->scope = DET::Scope::makeWithParentScope(scope);
  info->scope->isLoopScope = true;

  info->counterType = counterType->fullDetail(info->scope);
  info->start = start->fullDetail(info->scope);
  info->end = end->fullDetail(info->scope);

  info->counter = std::make_shared<DET::Variable>(counterName, info->counterType->type, info->scope);
  info->scope->items.push_back(info->counter);

  info->body = body->fullDetail(info->scope);
  return info;
};

ALTACORE_AST_VALIDATE_D(RangedForLoopStatement) {
  ALTACORE_VS_S(RangedForLoopStatement);
  if (counterName.empty()) ALTACORE_VALIDATION_ERROR("empty counter name for ranged `for` loop");
  if (!counterType) ALTACORE_VALIDATION_ERROR("empty counter type for ranged `for` loop");
  counterType->validate(stack, info->counterType);
  if (!start) ALTACORE_VALIDATION_ERROR("empty start for ranged `for` loop");
  start->validate(stack, info->start);
  if (!end) ALTACORE_VALIDATION_ERROR("empty end for ranged `for` loop");
  end->validate(stack, info->end);
  if (!body) ALTACORE_VALIDATION_ERROR("empty body for ranged `for` loop");
  body->validate(stack, info->body);
  ALTACORE_VS_E;
};
