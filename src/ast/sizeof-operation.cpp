#include "../../include/altacore/ast/sizeof-operation.hpp"

const AltaCore::AST::NodeType AltaCore::AST::SizeofOperation::nodeType() {
  return NodeType::SizeofOperation;
};

ALTACORE_AST_DETAIL_D(SizeofOperation) {
  ALTACORE_MAKE_DH(SizeofOperation);
  info->target = target->fullDetail(scope);
  return info;
};

ALTACORE_AST_VALIDATE_D(SizeofOperation) {
  ALTACORE_VS_S(SizeofOperation);
  if (!target) ALTACORE_VALIDATION_ERROR("Sizeof operation must contain a target node");
  target->validate(stack, info->target);
  ALTACORE_VS_E;
};
