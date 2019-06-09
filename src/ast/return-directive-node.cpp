#include "../../include/altacore/ast/return-directive-node.hpp"
#include "../../include/altacore/util.hpp"

const AltaCore::AST::NodeType AltaCore::AST::ReturnDirectiveNode::nodeType() {
  return NodeType::ReturnDirectiveNode;
};

AltaCore::AST::ReturnDirectiveNode::ReturnDirectiveNode(std::shared_ptr<AltaCore::AST::ExpressionNode> _expression):
  expression(_expression)
  {};

ALTACORE_AST_DETAIL_D(ReturnDirectiveNode) {
  ALTACORE_MAKE_DH(ReturnDirectiveNode);
  if (expression != nullptr) {
    auto func = Util::getFunction(scope).lock();
    info->parentFunction = func;
    info->expression = expression->fullDetail(scope);
  }
  return info;
};

ALTACORE_AST_VALIDATE_D(ReturnDirectiveNode) {
  ALTACORE_VS_S(ReturnDirectiveNode);
  if (expression) {
    expression->validate(stack, info->expression);
  }
  ALTACORE_VS_E;
};
