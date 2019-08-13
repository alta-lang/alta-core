#include "../../include/altacore/det/class.hpp"
#include "../../include/altacore/det/variable.hpp"
#include "../../include/altacore/ast/class-definition-node.hpp"
#include "../../include/altacore/det/scope.hpp"

const AltaCore::DET::NodeType AltaCore::DET::Class::nodeType() {
  return NodeType::Class;
};

std::shared_ptr<AltaCore::DET::Node> AltaCore::DET::Class::clone() {
  return std::make_shared<Class>(*this);
};

std::shared_ptr<AltaCore::DET::Node> AltaCore::DET::Class::deepClone() {
  auto self = std::dynamic_pointer_cast<Class>(clone());
  self->scope = std::dynamic_pointer_cast<Scope>(scope->deepClone());

  return self;
};

std::shared_ptr<AltaCore::DET::Class> AltaCore::DET::Class::create(std::string name, std::shared_ptr<AltaCore::DET::Scope> parentScope, std::vector<std::shared_ptr<Class>> parents, bool isStructure) {
  auto klass = std::make_shared<Class>(name, parentScope, parents);
  klass->scope = std::make_shared<Scope>(klass);
  auto thisType = std::make_shared<Type>(klass, std::vector<uint8_t> { (uint8_t)Shared::TypeModifierFlag::Reference });
  klass->isStructure = isStructure;

  if (!isStructure) {
    klass->scope->items.push_back(std::make_shared<Variable>("this", thisType, klass->scope));
  }

  return klass;
};

AltaCore::DET::Class::Class(std::string _name, std::shared_ptr<AltaCore::DET::Scope> _parentScope, std::vector<std::shared_ptr<Class>> _parents):
  ScopeItem(_name, _parentScope),
  parents(_parents)
  {};

bool AltaCore::DET::Class::hasParent(std::shared_ptr<Class> parent) const {
  for (auto& myParent: parents) {
    if (myParent->id == parent->id) return true;
    if (myParent->hasParent(parent)) return true;
  }
  return false;
};

std::shared_ptr<AltaCore::DET::Class> AltaCore::DET::Class::instantiateGeneric(std::vector<std::shared_ptr<Type>> genericArguments) {
  if (auto klass = ast.lock()) {
    auto inf = info.lock();
    if (!inf) {
      return nullptr;
    }
    return klass->instantiateGeneric(inf, genericArguments);
  } else {
    return nullptr;
  }
};

std::shared_ptr<AltaCore::DET::Function> AltaCore::DET::Class::findFromCast(const Type& target) {
  for (auto& from: fromCasts) {
    if (*std::get<1>(from->parameters[0]) == target) return from;
  }
  for (auto& from: fromCasts) {
    if (*std::get<1>(from->parameters[0]) % target) return from;
  }
  return nullptr;
};

std::shared_ptr<AltaCore::DET::Function> AltaCore::DET::Class::findToCast(const Type& target) {
  for (auto& to: toCasts) {
    if (target == *to->returnType) return to;
  }
  for (auto& to: toCasts) {
    if (target % *to->returnType) return to;
  }
  return nullptr;
};
