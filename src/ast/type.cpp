#include "../../include/altacore/ast/type.hpp"

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
    } else {
      DET::NativeType nt;

      if (name == "int") {
        nt = DET::NativeType::Integer;
      } else if (name == "byte" || name == "char") {
        nt = DET::NativeType::Byte;
      } else if (name == "bool") {
        nt = DET::NativeType::Bool;
      } else if (name == "void") {
        nt = DET::NativeType::Void;
      } else {
        throw std::runtime_error("non-native types aren't currently supported");
      }

      $type = std::make_shared<DET::Type>(nt, modifiers);
    }
  }
};
