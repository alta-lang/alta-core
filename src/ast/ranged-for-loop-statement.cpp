#include "../../include/altacore/ast/ranged-for-loop-statement.hpp"

const AltaCore::AST::NodeType AltaCore::AST::RangedForLoopStatement::nodeType() {
  return NodeType::RangedForLoopStatement;
};

ALTACORE_AST_DETAIL_D(RangedForLoopStatement) {
  ALTACORE_MAKE_DH(RangedForLoopStatement);
  info->wrapperScope = DET::Scope::makeWithParentScope(scope, position);
  info->scope = DET::Scope::makeWithParentScope(info->wrapperScope, position);
  info->scope->isLoopScope = true;

  info->counterType = counterType->fullDetail(info->wrapperScope);
  info->start = start->fullDetail(info->wrapperScope);
  if (end) {
    info->end = end->fullDetail(info->wrapperScope);
  } else {
    auto type = DET::Type::getUnderlyingType(info->start.get());
    if (!type->klass) ALTACORE_DETAILING_ERROR("Ranged-for loop target is not an iterator");
    if (type->pointerLevel() > 0) ALTACORE_DETAILING_ERROR("Ranged-for loop target must be an iterator or a reference to an iterator; pointers are not allowed");
    for (auto& item: type->klass->scope->items) {
      if (auto func = std::dynamic_pointer_cast<DET::Function>(item)) {
        if (func->parameters.size() != 0) continue;
        if (func->name == "next") {
          if (*func->returnType == DET::Type(DET::NativeType::Void)) continue;
          if (func->isAccessor) continue;
          if (info->next) ALTACORE_DETAILING_ERROR("Ranged-for loop iterator - next already found");
          info->next = func;
        }
      }
    }
    if (!info->next) ALTACORE_DETAILING_ERROR("Ranged-for loop target is not an iterator");
    info->generatorType = type;
  }

  info->counter = std::make_shared<DET::Variable>(counterName, info->counterType->type, position, info->scope);
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
  if (end) {
    end->validate(stack, info->end);
  }
  if (!body) ALTACORE_VALIDATION_ERROR("empty body for ranged `for` loop");
  body->validate(stack, info->body);
  ALTACORE_VS_E;
};
