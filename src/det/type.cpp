#include "../../include/altacore/det/type.hpp"
#include "../../include/altacore/ast.hpp"

const AltaCore::DET::NodeType AltaCore::DET::Type::nodeType() {
  return NodeType::Type;
};

std::shared_ptr<AltaCore::DET::Node> AltaCore::DET::Type::clone() {
  return std::make_shared<Type>(*this);
};

std::shared_ptr<AltaCore::DET::Node> AltaCore::DET::Type::deepClone() {
  return clone();
};

std::shared_ptr<AltaCore::DET::Type> AltaCore::DET::Type::getUnderlyingType(AltaCore::AST::ExpressionNode* expression) {
  using ExpressionType = AST::NodeType;
  using Modifier = AST::TypeModifierFlag;
  ExpressionType exprType = expression->nodeType();

  if (exprType == ExpressionType::IntegerLiteralNode) {
    auto intLit = dynamic_cast<AST::IntegerLiteralNode*>(expression);
    if (intLit == nullptr) throw std::runtime_error("wut da heck");
    return std::make_shared<Type>(NativeType::Integer, std::vector<uint8_t> { (uint8_t)Modifier::Constant });
  } else if (exprType == ExpressionType::VariableDefinitionExpression) {
    auto varDef = dynamic_cast<AST::VariableDefinitionExpression*>(expression);
    if (varDef == nullptr) throw std::runtime_error("no.");
    return std::dynamic_pointer_cast<Type>(varDef->$variable->type->clone())->reference();
  } else if (exprType == ExpressionType::AssignmentExpression) {
    auto assign = dynamic_cast<AST::AssignmentExpression*>(expression);
    if (assign == nullptr) throw std::runtime_error("nuh-uh.");
    return getUnderlyingType(assign->target.get());
  } else if (exprType == ExpressionType::Fetch) {
    auto fetch = dynamic_cast<AST::Fetch*>(expression);
    if (fetch == nullptr) throw std::runtime_error("NOPE");
    auto item = fetch->$item.get();
    if (item->nodeType() == NodeType::Variable) {
      auto var = dynamic_cast<Variable*>(item);
      return std::dynamic_pointer_cast<Type>(var->type->clone());
    } else if (item->nodeType() == NodeType::Function) {
      auto func = dynamic_cast<Function*>(item);
      throw std::runtime_error("function types are not supported at the moment");
    } else {
      throw std::runtime_error("Only functions and variables have underlying types");
    }
  }

  return nullptr;
};

std::shared_ptr<AltaCore::DET::Type> AltaCore::DET::Type::reference() {
  modifiers.push_back((uint8_t)Shared::TypeModifierFlag::Reference);
  return shared_from_this();
};
std::shared_ptr<AltaCore::DET::Type> AltaCore::DET::Type::dereference() {
  if (modifiers.size() > 0) {
    auto idx = modifiers.size() - 1;
    modifiers[idx] &= ~(uint8_t)Shared::TypeModifierFlag::Reference;
    // `pop_back` if the modifier level is now empty
    // why keep around a useless entry in the vector?
    if (modifiers[idx] == 0) {
      modifiers.pop_back();
    }
  }
  return shared_from_this();
};
std::shared_ptr<AltaCore::DET::Type> AltaCore::DET::Type::point() {
  modifiers.push_back((uint8_t)Shared::TypeModifierFlag::Pointer);
  return shared_from_this();
};
std::shared_ptr<AltaCore::DET::Type> AltaCore::DET::Type::follow() {
  if (modifiers.size() > 0) {
    auto idx = modifiers.size() - 1;
    modifiers[idx] &= ~(uint8_t)Shared::TypeModifierFlag::Pointer;
    if (modifiers[idx] == 0) {
      modifiers.pop_back();
    }
  }
  return shared_from_this();
};
std::shared_ptr<AltaCore::DET::Type> AltaCore::DET::Type::followBlindly() {
  if (modifiers.size() > 0) {
    auto idx = modifiers.size() - 1;
    modifiers[idx] &= ~((uint8_t)Shared::TypeModifierFlag::Reference | (uint8_t)Shared::TypeModifierFlag::Pointer);
    if (modifiers[idx] == 0) {
      modifiers.pop_back();
    }
  }
  return shared_from_this();
};

AltaCore::DET::Type::Type(AltaCore::DET::NativeType _nativeTypeName, std::vector<uint8_t> _modifiers):
  isNative(true),
  nativeTypeName(_nativeTypeName),
  modifiers(_modifiers)
  {};
