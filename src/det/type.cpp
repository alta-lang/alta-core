#include "../include/altacore/det/type.hpp"
#include "../include/altacore/ast.hpp"

const AltaCore::DET::NodeType AltaCore::DET::Type::nodeType() {
  return NodeType::Type;
};

AltaCore::DET::Type* AltaCore::DET::Type::clone() {
  return new Type(*this);
};

AltaCore::DET::Type* AltaCore::DET::Type::deepClone() {
  return clone();
};

AltaCore::DET::Type* AltaCore::DET::Type::getUnderlyingType(AltaCore::AST::ExpressionNode* expression) {
  using ExpressionType = AST::NodeType;
  using Modifier = AST::TypeModifierFlag;
  ExpressionType exprType = expression->nodeType();

  if (exprType == ExpressionType::IntegerLiteralNode) {
    auto intLit = dynamic_cast<AST::IntegerLiteralNode*>(expression);
    if (intLit == nullptr) throw std::runtime_error("wut da heck");
    return new Type(NativeType::Integer, { (uint8_t)Modifier::Constant });
  } else if (exprType == ExpressionType::VariableDefinitionExpression) {
    auto varDef = dynamic_cast<AST::VariableDefinitionExpression*>(expression);
    if (varDef == nullptr) throw std::runtime_error("no.");
    return varDef->$variable->type->deepClone()->reference();
  }

  return nullptr;
};

AltaCore::DET::Type* AltaCore::DET::Type::reference() {
  modifiers.push_back((uint8_t)Shared::TypeModifierFlag::Reference);
  return this;
};
AltaCore::DET::Type* AltaCore::DET::Type::dereference() {
  if (modifiers.size() > 0) {
    auto idx = modifiers.size() - 1;
    modifiers[idx] &= ~(uint8_t)Shared::TypeModifierFlag::Reference;
    // `pop_back` if the modifier level is now empty
    // why keep around a useless entry in the vector?
    if (modifiers[idx] == 0) {
      modifiers.pop_back();
    }
  }
  return this;
};
AltaCore::DET::Type* AltaCore::DET::Type::point() {
  modifiers.push_back((uint8_t)Shared::TypeModifierFlag::Pointer);
  return this;
};
AltaCore::DET::Type* AltaCore::DET::Type::follow() {
  if (modifiers.size() > 0) {
    auto idx = modifiers.size() - 1;
    modifiers[idx] &= ~(uint8_t)Shared::TypeModifierFlag::Pointer;
    if (modifiers[idx] == 0) {
      modifiers.pop_back();
    }
  }
  return this;
};
AltaCore::DET::Type* AltaCore::DET::Type::followBlindly() {
  if (modifiers.size() > 0) {
    auto idx = modifiers.size() - 1;
    modifiers[idx] &= ~((uint8_t)Shared::TypeModifierFlag::Reference | (uint8_t)Shared::TypeModifierFlag::Pointer);
    if (modifiers[idx] == 0) {
      modifiers.pop_back();
    }
  }
  return this;
};

AltaCore::DET::Type::Type(AltaCore::DET::NativeType _nativeTypeName, std::vector<uint8_t> _modifiers):
  isNative(true),
  nativeTypeName(_nativeTypeName),
  modifiers(_modifiers)
  {};