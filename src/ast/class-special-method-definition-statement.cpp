#include "../../include/altacore/ast/class-special-method-definition-statement.hpp"

const AltaCore::AST::NodeType AltaCore::AST::ClassSpecialMethodDefinitionStatement::nodeType() {
  return NodeType::ClassSpecialMethodDefinitionStatement;
};

AltaCore::AST::ClassSpecialMethodDefinitionStatement::ClassSpecialMethodDefinitionStatement(AltaCore::AST::Visibility _visibilityModifier, AltaCore::AST::SpecialClassMethod _type):
  visibilityModifier(_visibilityModifier),
  type(_type)
  {};

void AltaCore::AST::ClassSpecialMethodDefinitionStatement::detail(std::shared_ptr<AltaCore::DET::Scope> scope) {
  $klass = scope->parentClass.lock();
  if (type == SpecialClassMethod::Constructor) {
    std::vector<std::tuple<std::string, std::shared_ptr<AltaCore::DET::Type>, bool, std::string>> params;

    for (auto& param: parameters) {
      param->detail(scope, false);
      params.push_back(std::make_tuple(param->name, param->type->$type, param->isVariable, param->id));
    }

    $method = DET::Function::create(scope, "constructor", params, std::make_shared<DET::Type>(DET::NativeType::Void));

    $method->visibility = visibilityModifier;

    body->detail($method->scope);
  } else {
    throw std::runtime_error("destructors aren't supported yet");
  }
};

ALTACORE_AST_VALIDATE_D(ClassSpecialMethodDefinitionStatement) {
  
};