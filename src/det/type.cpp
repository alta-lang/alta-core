#include "../../include/altacore/det/type.hpp"
#include "../../include/altacore/ast.hpp"
#include "../../include/altacore/util.hpp"

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
    throw std::runtime_error("fetches have multiple possible types. use `AltaCore::DET::Type::getUnderlyingTypes` instead");
  } else if (exprType == ExpressionType::BooleanLiteralNode) {
    return std::make_shared<Type>(NativeType::Bool, std::vector<uint8_t> { (uint8_t)Modifier::Constant });
  } else if (exprType == ExpressionType::BinaryOperation) {
    auto binOp = dynamic_cast<AST::BinaryOperation*>(expression);
    if (binOp == nullptr) throw std::runtime_error("wah.");
    return getUnderlyingType(binOp->left.get());
  } else if (exprType == ExpressionType::FunctionCallExpression) {
    auto call = dynamic_cast<AST::FunctionCallExpression*>(expression);
    if (call == nullptr) throw std::runtime_error("bro wut dah heck.");
    return call->$targetType->returnType;
  } else if (exprType == ExpressionType::Accessor) {
    throw std::runtime_error("accessors have multiple possible types. use `AltaCore::DET::Type::getUnderlyingTypes` instead");
  } else if (exprType == ExpressionType::StringLiteralNode) {
    return std::make_shared<Type>(NativeType::Byte, std::vector<uint8_t> { (uint8_t)Modifier::Constant | (uint8_t)Modifier::Pointer, (uint8_t)Modifier::Constant });
  }

  return nullptr;
};
std::shared_ptr<AltaCore::DET::Type> AltaCore::DET::Type::getUnderlyingType(std::shared_ptr<AltaCore::DET::ScopeItem> item) {
  using ItemType = DET::NodeType;
  using Modifier = AST::TypeModifierFlag;
  ItemType itemType = item->nodeType();

  if (itemType == ItemType::Function) {
    auto func = std::dynamic_pointer_cast<Function>(item);
    std::vector<std::pair<std::string, std::shared_ptr<Type>>> params;
    for (auto& [name, type]: func->parameters) {
      params.push_back(std::make_pair(name, type));
    }
    return std::make_shared<Type>(func->returnType, params);
  } else if (itemType == ItemType::Variable) {
    auto var = std::dynamic_pointer_cast<Variable>(item);
    return std::dynamic_pointer_cast<Type>(var->type);
  } else {
    throw std::runtime_error("Only functions and variables have underlying types");
  }
};
std::vector<std::shared_ptr<AltaCore::DET::Type>> AltaCore::DET::Type::getUnderlyingTypes(AltaCore::AST::ExpressionNode* expression) {
  using ExpressionType = AST::NodeType;
  using Modifier = AST::TypeModifierFlag;
  ExpressionType exprType = expression->nodeType();

  if (exprType == ExpressionType::Fetch) {
    auto fetch = dynamic_cast<AST::Fetch*>(expression);
    if (fetch == nullptr) throw std::runtime_error("NOPE");
    std::vector<std::shared_ptr<Type>> types;
    for (auto& item: fetch->$items) {
      types.push_back(getUnderlyingType(item));
    }
    return types;
  } else if (exprType == ExpressionType::Accessor) {
    auto acc = dynamic_cast<AST::Accessor*>(expression);
    if (acc == nullptr) throw std::runtime_error("expression invalidly identified as an AltaCore::AST::Accessor");
    std::vector<std::shared_ptr<Type>> types;
    for (auto& item: acc->$items) {
      types.push_back(getUnderlyingType(item));
    }
    return types;
  }

  auto type = getUnderlyingType(expression);
  if (type == nullptr) return {};
  return { type };
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
const size_t AltaCore::DET::Type::indirectionLevel() const {
  size_t count = 0;

  for (auto& modLevel: modifiers) {
    if (modLevel & (uint8_t)Shared::TypeModifierFlag::Pointer || modLevel & (uint8_t)Shared::TypeModifierFlag::Reference) {
      count++;
    }
  }

  return count;
};
const size_t AltaCore::DET::Type::pointerLevel() const {
  size_t count = 0;

  for (auto& modLevel: modifiers) {
    if (modLevel & (uint8_t)Shared::TypeModifierFlag::Pointer) {
      count++;
    }
  }

  return count;
};
const size_t AltaCore::DET::Type::referenceLevel() const {
  size_t count = 0;

  for (auto& modLevel: modifiers) {
    if (modLevel & (uint8_t)Shared::TypeModifierFlag::Reference) {
      count++;
    }
  }

  return count;
};

size_t AltaCore::DET::Type::compatiblity(const AltaCore::DET::Type& other) {
  size_t compat = 1;

  if (isExactlyCompatibleWith(other)) return SIZE_MAX;
  if (!commonCompatiblity(other)) return 0;

  if (referenceLevel() == other.referenceLevel()) {
    compat++;
  }

  if (modifiers.size() == other.modifiers.size()) {
    bool modsEqual = true;
    for (size_t i = 0; i < modifiers.size(); i++) {
      if (modifiers[i] != other.modifiers[i]) {
        modsEqual = false;
        break;
      }
    }
    if (modsEqual) {
      compat++;
    }
  }

  if (isFunction) {
    auto retCompat = returnType->compatiblity(*other.returnType);
    if (retCompat == 0) return 0;
    compat += retCompat;
    if (parameters.size() != other.parameters.size()) return 0;
    for (size_t i = 0; i < parameters.size(); i++) {
      auto paramCompat = parameters[i].second->compatiblity(*other.parameters[i].second);
      if (paramCompat == 0) return 0;
      compat += paramCompat;
    }
  } else {
    if (nativeTypeName == other.nativeTypeName) {
      compat++;
    }
  }

  return compat;
};

bool AltaCore::DET::Type::commonCompatiblity(const AltaCore::DET::Type& other) {
  if (isFunction != other.isFunction) return false;
  if (isNative != other.isNative) return false;
  if (pointerLevel() != other.pointerLevel()) return false;

  /**
   * here's the reasoning behind this check:
   * 
   * the current type is the destination type (e.g. the type of a function parameter)
   * the `other` type is the source type (e.g. the type of an argument in a function call)
   * 
   * we can automatically ref the source type *once* if needed, but no more than once
   * (because you can't get the address of an address)
   */
  if (referenceLevel() > other.referenceLevel() + 1) return false;
  /**
   * why don't we also check the reference level the other way around?
   * because we can deref the source type as many times as we need
   * (because in C, for example, you can just keep following pointers as many times as you need to)
   */

  if (isFunction && parameters.size() != other.parameters.size()) return false;

  return true;
};

bool AltaCore::DET::Type::isExactlyCompatibleWith(const AltaCore::DET::Type& other) {
  if (!commonCompatiblity(other)) return false;

  // here, we care about *exact* compatability, and that includes all modifiers
  if (modifiers.size() != other.modifiers.size()) return false;
  for (size_t i = 0; i < modifiers.size(); i++) {
    if (modifiers[i] != other.modifiers[i]) return false;
  }

  if (isFunction) {
    if (!returnType->isExactlyCompatibleWith(*other.returnType)) return false;
    for (size_t i = 0; i < parameters.size(); i++) {
      if (!parameters[i].second->isExactlyCompatibleWith(*other.parameters[i].second)) return false;
    }
  } else {
    if (nativeTypeName != other.nativeTypeName) return false;
  }

  return true;
};

bool AltaCore::DET::Type::isCompatibleWith(const AltaCore::DET::Type& other) {
  if (!commonCompatiblity(other)) return false;

  if (isFunction) {
    if (!returnType->isCompatibleWith(*other.returnType)) return false;
    if (parameters.size() != other.parameters.size()) return false;
    for (size_t i = 0; i < parameters.size(); i++) {
      if (!parameters[i].second->isCompatibleWith(*other.parameters[i].second)) return false;
    }
  } else {
    // only check for void
    // all other native types are integral and can be coerced to each other
  }

  return true;
};

AltaCore::DET::Type::Type(AltaCore::DET::NativeType _nativeTypeName, std::vector<uint8_t> _modifiers):
  isNative(true),
  isFunction(false),
  nativeTypeName(_nativeTypeName),
  modifiers(_modifiers)
  {};
AltaCore::DET::Type::Type(std::shared_ptr<AltaCore::DET::Type> _returnType, std::vector<std::pair<std::string, std::shared_ptr<AltaCore::DET::Type>>> _parameters, std::vector<uint8_t> _modifiers):
  isNative(true),
  isFunction(true),
  returnType(_returnType),
  parameters(_parameters),
  modifiers(_modifiers)
  {};

bool AltaCore::DET::Type::operator %(const AltaCore::DET::Type& other) {
  return isCompatibleWith(other);
};
