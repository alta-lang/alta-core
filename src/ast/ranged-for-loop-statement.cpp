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
  if (end) {
    info->end = end->fullDetail(info->scope);
  }

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
  if (end) {
    end->validate(stack, info->end);
  } else {
    auto type = DET::Type::getUnderlyingType(info->start.get());
    if (!type->klass) ALTACORE_VALIDATION_ERROR("Ranged-for loop target is not an iterator");
    for (auto& item: type->klass->scope->items) {
      if (auto func = std::dynamic_pointer_cast<DET::Function>(item)) {
        if (func->parameters.size() != 0) continue;
        if (func->name == "next") {
          if (*func->returnType == DET::Type(DET::NativeType::Void)) continue;
          if (func->isAccessor) continue;
          if (info->next) throw std::runtime_error("Ranged-for loop iterator - next already found");
          info->next = func;
        } else if (func->name == "done") {
          if (func->returnType->nativeTypeName != DET::NativeType::Bool) continue;
          if (func->returnType->pointerLevel() != 0) continue;
          if (!func->isAccessor) continue;
          if (info->done) throw std::runtime_error("Ranged-for loop iterator - done already found");
          info->done = func;
        }
      } else if (auto var = std::dynamic_pointer_cast<DET::Variable>(item)) {
        if (var->name != "done") continue;
        if (var->type->nativeTypeName != DET::NativeType::Bool) continue;
        if (var->type->pointerLevel() != 0) continue;
        if (info->done) throw std::runtime_error("Ranged-for loop iterator - done already found");
        info->done = var;
      }
    }
    if (!(info->next && info->done)) ALTACORE_VALIDATION_ERROR("Ranged-for loop target is not an iterator");
    info->generatorType = type;
  }
  if (!body) ALTACORE_VALIDATION_ERROR("empty body for ranged `for` loop");
  body->validate(stack, info->body);
  ALTACORE_VS_E;
};
