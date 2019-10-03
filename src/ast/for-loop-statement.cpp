#include "../../include/altacore/ast/for-loop-statement.hpp"

const AltaCore::AST::NodeType AltaCore::AST::ForLoopStatement::nodeType() {
  return NodeType::ForLoopStatement;
};

ALTACORE_AST_DETAIL_D(ForLoopStatement) {
  ALTACORE_MAKE_DH(ForLoopStatement);
  info->scope = DET::Scope::makeWithParentScope(scope);
  info->scope->isLoopScope = true;

  info->initializer = initializer->fullDetail(info->scope);
  info->condition = condition->fullDetail(info->scope);
  info->increment = increment->fullDetail(info->scope);
  info->body = body->fullDetail(info->scope);
  return info;
};

ALTACORE_AST_VALIDATE_D(ForLoopStatement) {
  ALTACORE_VS_S(ForLoopStatement);
  if (!body) ALTACORE_VALIDATION_ERROR("empty body for `for` loop");
  body->validate(stack, info->body);
  ALTACORE_VS_E;
};
