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

std::shared_ptr<AltaCore::DH::Node> AltaCore::AST::Type::detail(std::shared_ptr<AltaCore::DET::Scope> scope, bool hoist) {
  ALTACORE_MAKE_DH(Type);
  if (isFunction) {
    std::vector<std::tuple<std::string, std::shared_ptr<DET::Type>, bool, std::string>> detParams;
    info->returnType = returnType->fullDetail(scope);
    for (auto& [param, isVariable, id]: parameters) {
      auto det = param->fullDetail(scope);
      info->parameters.push_back(det);
      detParams.push_back({ "", det->type, isVariable, id });
    }
    info->type = std::make_shared<DET::Type>(info->returnType->type, detParams, modifiers);
    if (hoist) scope->hoist(info->type);
  } else if (!_injected_type) {
    std::shared_ptr<DET::ScopeItem> item = nullptr;

    if (lookup) {
      info->lookup = lookup->fullDetail(scope);
      auto nodeT = lookup->nodeType();
      std::shared_ptr<DET::Class> klass = nullptr;

      if (nodeT == NodeType::Fetch) {
        auto fetch = std::dynamic_pointer_cast<Fetch>(lookup);
        auto fetchDet = std::dynamic_pointer_cast<DH::Fetch>(info->lookup);
        if (!fetchDet->narrowedTo) {
          ALTACORE_DETAILING_ERROR("that's weird, classes should be narrowed");
        }
        item = fetchDet->narrowedTo;
      } else if (nodeT == NodeType::Accessor) {
        auto acc = std::dynamic_pointer_cast<Accessor>(lookup);
        auto accDet = std::dynamic_pointer_cast<DH::Accessor>(info->lookup);
        if (!accDet->narrowedTo) {
          ALTACORE_DETAILING_ERROR("that's weird, classes should be narrowed");
        }
        item = accDet->narrowedTo;
      } else {
        throw std::runtime_error("WTF NO DONT DO THAT");
      }
    }

    if (item && item->nodeType() == DET::NodeType::Type) {
      info->type = std::dynamic_pointer_cast<DET::Type>(item)->copy();
      info->type->modifiers.insert(info->type->modifiers.begin(), modifiers.begin(), modifiers.end());
    } else if (isAny) {
      info->type = std::make_shared<DET::Type>();
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

      info->type = std::make_shared<DET::Type>(nt, modifiers);
    } else {
      std::shared_ptr<DET::Class> klass = nullptr;

      if (item) {
        klass = std::dynamic_pointer_cast<DET::Class>(item);
      } else {
        throw std::runtime_error("WTF NO DONT DO THAT");
      }

      if (!klass) {
        throw std::runtime_error("y du u du this");
      }

      info->type = std::make_shared<DET::Type>(klass, modifiers);
    }
  } else {
    info->type = _injected_type;
  }
  return info;
};

ALTACORE_AST_VALIDATE_D(Type) {
  ALTACORE_VS_S(Type);
  if (isFunction) {
    if (isAny) ALTACORE_VALIDATION_ERROR("type can't be multiple kinds, only one of `function`, `any`, or `native`");
    if (!returnType) ALTACORE_VALIDATION_ERROR("empty return type for function type");
    returnType->validate(stack, info->returnType);
    for (size_t i = 0; i < parameters.size(); i++) {
      auto& [type, isVariable, name] = parameters[i];
      auto& paramDet = info->parameters[i];
      if (!type) ALTACORE_VALIDATION_ERROR("empty type for parameter for function type");
      type->validate(stack, paramDet);
    }
  } else {
    if (returnType) ALTACORE_VALIDATION_ERROR("non-function types can't have return types");
    if (parameters.size() > 0) ALTACORE_VALIDATION_ERROR("non-function types can't have parameters");
    if (isAny) {
      //if (isNative) ALTACORE_VALIDATION_ERROR("type can't be multiple kinds, only one of `function`, `any`, or `native`");
      if (!name.empty()) ALTACORE_VALIDATION_ERROR("`any` type can't have a native type name");
      if (lookup) ALTACORE_VALIDATION_ERROR("`any` type can't have a target lookup");
    } else if (isNative) {
      //if (isAny) ALTACORE_VALIDATION_ERROR("type can't be multiple kinds, only one of `function`, `any`, or `native`");
      if (name.empty()) ALTACORE_VALIDATION_ERROR("empty name for native type");
      if (lookup) ALTACORE_VALIDATION_ERROR("native type can't have a target lookup");
    } else {
      if (!name.empty()) ALTACORE_VALIDATION_ERROR("non-native type can't have a native type name");
      if (!lookup) ALTACORE_VALIDATION_ERROR("empty lookup for non-native type");
      lookup->validate(stack, info->lookup);
      for (auto& mod: modifiers) {
        if (mod >= (uint8_t)TypeModifierFlag::Signed) {
          ALTACORE_VALIDATION_ERROR("only native types can be `signed`, `unsigned`, `long`, or `short`");
        }
      }
    }
  }
  ALTACORE_VS_E;
};
