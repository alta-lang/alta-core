#include "../../include/altacore/ast/class-method-definition-statement.hpp"
#include "../../include/altacore/det/type.hpp"

const AltaCore::AST::NodeType AltaCore::AST::ClassMethodDefinitionStatement::nodeType() {
  return NodeType::ClassMethodDefinitionStatement;
};

AltaCore::AST::ClassMethodDefinitionStatement::ClassMethodDefinitionStatement(AltaCore::AST::Visibility _visibilityModifier):
  visibilityModifier(_visibilityModifier)
  {};

void AltaCore::AST::ClassMethodDefinitionStatement::detail(std::shared_ptr<AltaCore::DET::Scope> scope) {
  if (funcDef == nullptr) throw std::runtime_error("stop that");
  funcDef->detail(scope);
  funcDef->$function->visibility = visibilityModifier;
  funcDef->$function->isMethod = true;
  auto klass = scope->parentClass.lock();
  funcDef->$function->parentClassType = std::make_shared<DET::Type>(klass, std::vector<uint8_t> { (uint8_t)TypeModifierFlag::Reference });
};
