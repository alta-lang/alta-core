#include "../../include/altacore/ast/type.hpp"
#include "../../include/altacore/ast/fetch.hpp"
#include "../../include/altacore/ast/accessor.hpp"
#include "../../include/altacore/det/class.hpp"

const AltaCore::AST::NodeType AltaCore::AST::Type::nodeType() {
  return NodeType::Type;
};

AltaCore::AST::Type::Type(std::string _name, std::vector<uint8_t> _modifiers):
  name(_name),
  modifiers(_modifiers)
  {};
AltaCore::AST::Type::Type(std::shared_ptr<AltaCore::AST::Type> _returnType, std::vector<std::tuple<std::shared_ptr<AltaCore::AST::Type>, bool, std::string>> _parameters, std::vector<uint8_t> _modifiers):
  isFunction(true),
  returnType(_returnType),
  parameters(_parameters),
  modifiers(_modifiers)
  {};

void AltaCore::AST::Type::detail(std::shared_ptr<AltaCore::DET::Scope> scope, bool hoist) {
  if (isFunction) {
    std::vector<std::tuple<std::string, std::shared_ptr<DET::Type>, bool, std::string>> detParams;
    returnType->detail(scope);
    for (auto& [param, isVariable, id]: parameters) {
      param->detail(scope);
      detParams.push_back({ "", param->$type, isVariable, id });
    }
    $type = std::make_shared<DET::Type>(returnType->$type, detParams, modifiers);
    if (hoist) scope->hoist($type);
  } else {
    if (isAny) {
      $type = std::make_shared<DET::Type>();
    } else if (isNative) {
      DET::NativeType nt;

      if (name == "int") {
        nt = DET::NativeType::Integer;
      } else if (name == "byte" || name == "char") {
        nt = DET::NativeType::Byte;
      } else if (name == "bool") {
        nt = DET::NativeType::Bool;
      } else if (name == "void") {
        nt = DET::NativeType::Void;
      }

      $type = std::make_shared<DET::Type>(nt, modifiers);
    } else {
      lookup->detail(scope);

      auto nodeT = lookup->nodeType();
      std::shared_ptr<DET::Class> klass = nullptr;

      if (nodeT == NodeType::Fetch) {
        auto fetch = std::dynamic_pointer_cast<Fetch>(lookup);
        if (!fetch->$narrowedTo) {
          throw std::runtime_error("that's weird, classes should be narrowed");
        }
        klass = std::dynamic_pointer_cast<DET::Class>(fetch->$narrowedTo);
      } else if (nodeT == NodeType::Accessor) {
        auto acc = std::dynamic_pointer_cast<Accessor>(lookup);
        if (!acc->$narrowedTo) {
          throw std::runtime_error("that's weird, classes should be narrowed");
        }
        klass = std::dynamic_pointer_cast<DET::Class>(acc->$narrowedTo);
      } else {
        throw std::runtime_error("WTF NO DONT DO THAT");
      }

      if (!klass) {
        throw std::runtime_error("y du u du this");
      }

      $type = std::make_shared<DET::Type>(klass, modifiers);
    }
  }
};

ALTACORE_AST_VALIDATE_D(Type) {
  ALTACORE_VS_S;
  if (isFunction) {
    if (isAny) throw ValidationError("type can't be multiple kinds, only one of `function`, `any`, or `native`");
    if (!returnType) throw ValidationError("empty return type for function type");
    returnType->validate(stack);
    for (auto& [type, isVariable, name]: parameters) {
      if (!type) throw ValidationError("empty type for parameter for function type");
      type->validate(stack);
    }
  } else {
    if (returnType) throw ValidationError("non-function types can't have return types");
    if (parameters.size() > 0) throw ValidationError("non-function types can't have parameters");
    if (isAny) {
      //if (isNative) throw ValidationError("type can't be multiple kinds, only one of `function`, `any`, or `native`");
      if (!name.empty()) throw ValidationError("`any` type can't have a native type name");
      if (lookup) throw ValidationError("`any` type can't have a target lookup");
    } else if (isNative) {
      //if (isAny) throw ValidationError("type can't be multiple kinds, only one of `function`, `any`, or `native`");
      if (name.empty()) throw ValidationError("empty name for native type");
      if (lookup) throw ValidationError("native type can't have a target lookup");
    } else {
      if (!name.empty()) throw ValidationError("non-native type can't have a native type name");
      if (!lookup) throw ValidationError("empty lookup for non-native type");
      lookup->validate(stack);
    }
  }
  ALTACORE_VS_E;
};