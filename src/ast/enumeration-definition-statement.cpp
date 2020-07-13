#include "../../include/altacore/ast/enumeration-definition-statement.hpp"
#include "../../include/altacore/util.hpp"

const AltaCore::AST::NodeType AltaCore::AST::EnumerationDefinitionNode::nodeType() {
  return NodeType::EnumerationDefinitionNode;
};

ALTACORE_AST_DETAIL_D(EnumerationDefinitionNode) {
  ALTACORE_MAKE_DH(EnumerationDefinitionNode);

  info->ns = DET::Namespace::create(info->inputScope, name);
  info->inputScope->items.push_back(info->ns);
  info->isExport = std::find(modifiers.begin(), modifiers.end(), "export") != modifiers.end();

  if (info->isExport) {
    if (auto mod = Util::getModule(scope.get()).lock()) {
      mod->exports->items.push_back(info->ns);
    }
  }

  info->underlyingType = underlyingType->fullDetail(info->inputScope);
  info->ns->underlyingEnumerationType = info->underlyingType->type;

  info->memberType = info->underlyingType->type->copy();
  if (info->memberType->modifiers.size() == 0) {
    info->memberType->modifiers.push_back(0);
  }
  info->memberType->modifiers.front() |= (uint8_t)Shared::TypeModifierFlag::Constant;

  for (size_t i = 0; i < members.size(); ++i) {
    auto& [key, value] = members[i];
    info->memberDetails[key] = value ? value->fullDetail(info->ns->scope) : nullptr;
    info->memberVariables[key] = std::make_shared<DET::Variable>(key, info->memberType, info->ns->scope);
    info->ns->scope->items.push_back(info->memberVariables[key]);
    if (!value) {
      bool found = false;
      if (info->memberType->isNative || info->memberType->pointerLevel() > 0) {
        // it's integral, it's fine
        found = true;
      } else if (info->memberType->klass) {
        // it's got class 😎 (*get it*, because... puns... heh... no? just me? ok)
        auto intType = std::make_shared<DET::Type>(DET::NativeType::Integer);
        if (i == 0) {
          if (auto from = info->memberType->klass->findFromCast(*intType)) {
            info->memberOperators[key] = from;
            found = true;
          } else {
            ALTACORE_DETAILING_ERROR("Type was not integral and no integral `from` cast operator was found (i.e. no `from int` operator)");
          }
        } else if (auto op = info->memberType->klass->findOperator(Shared::ClassOperatorType::Addition, Shared::ClassOperatorOrientation::Left, intType)) {
          info->memberOperators[key] = op;
          found = true;
        }
      }
      if (!found) {
        ALTACORE_DETAILING_ERROR("Type was not integral and no integral addition operator was found (i.e. no `this + int: ...` operator)");
      }
    }
  }

  return info;
};

ALTACORE_AST_VALIDATE_D(EnumerationDefinitionNode) {
  ALTACORE_VS_S(EnumerationDefinitionNode);

  if (underlyingType)
    underlyingType->validate(stack, info->underlyingType);

  for (auto& [key, value]: members) {
    if (value)
      value->validate(stack, info->memberDetails[key]);
  }

  ALTACORE_VS_E;
};
