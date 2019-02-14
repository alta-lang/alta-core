#include "../../include/altacore/ast/attribute-statement.hpp"

const AltaCore::AST::NodeType AltaCore::AST::AttributeStatement::nodeType() {
  return NodeType::AttributeStatement;
};

AltaCore::AST::AttributeStatement::AttributeStatement(std::shared_ptr<AltaCore::AST::AttributeNode> _attribute):
  attribute(_attribute)
  {};

ALTACORE_AST_DETAIL_D(AttributeStatement) {
  ALTACORE_MAKE_DH(AttributeStatement);
  info->attribute = attribute->fullDetail(scope);
  return info;
};

ALTACORE_AST_VALIDATE_D(AttributeStatement) {
  ALTACORE_VS_S(AttributeStatement);
  attribute->validate(stack, info->attribute);
  ALTACORE_VS_E;
};
