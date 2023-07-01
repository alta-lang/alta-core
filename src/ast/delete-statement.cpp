#include "../../include/altacore/ast/delete-statement.hpp"

const AltaCore::AST::NodeType AltaCore::AST::DeleteStatement::nodeType() {
  return NodeType::DeleteStatement;
};

ALTACORE_AST_DETAIL_D(DeleteStatement) {
  ALTACORE_MAKE_DH(DeleteStatement);

  info->target = target->fullDetail(info->inputScope);
  info->persistent = persistent;
  info->targetType = AltaCore::DET::Type::getUnderlyingType(info->target.get());

  return info;
};

ALTACORE_AST_VALIDATE_D(DeleteStatement) {
  ALTACORE_VS_S(DeleteStatement);

  target->validate(stack, info->target);

  if (info->persistent && info->targetType->pointerLevel() == 0) {
    ALTACORE_VALIDATION_ERROR("persistent deletions must refer to a pointer type");
  }

  ALTACORE_VS_E;
};
