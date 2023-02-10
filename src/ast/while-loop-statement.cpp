#include "../../include/altacore/ast/while-loop-statement.hpp"

const AltaCore::AST::NodeType AltaCore::AST::WhileLoopStatement::nodeType() {
  return NodeType::WhileLoopStatement;
};

ALTACORE_AST_DETAIL_D(WhileLoopStatement) {
  ALTACORE_MAKE_DH(WhileLoopStatement);
  info->scope = DET::Scope::makeWithParentScope(scope, position);
  info->scope->isLoopScope = true;

  info->test = test->fullDetail(info->scope);
  info->body = body->fullDetail(info->scope);
  return info;
};

ALTACORE_AST_VALIDATE_D(WhileLoopStatement) {
  ALTACORE_VS_S(WhileLoopStatement);
  if (!test) ALTACORE_VALIDATION_ERROR("empty test for while loop");
  test->validate(stack, info->test);
  if (!body) ALTACORE_VALIDATION_ERROR("empty body for while loop");
  body->validate(stack, info->body);
  ALTACORE_VS_E;
};
