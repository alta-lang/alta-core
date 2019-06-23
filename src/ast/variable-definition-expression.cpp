#include "../../include/altacore/ast/variable-definition-expression.hpp"
#include "../../include/altacore/util.hpp"
#include "../../include/altacore/ast/nullptr-expression.hpp"

const AltaCore::AST::NodeType AltaCore::AST::VariableDefinitionExpression::nodeType() {
  return NodeType::VariableDefinitionExpression;
};

AltaCore::AST::VariableDefinitionExpression::VariableDefinitionExpression(
  std::string _name,
  std::shared_ptr<AltaCore::AST::Type> _type,
  std::shared_ptr<AltaCore::AST::ExpressionNode> _initializationExpression
):
  name(_name),
  type(_type),
  initializationExpression(_initializationExpression)
  {};

ALTACORE_AST_DETAIL_NO_BODY_OPT_D(VariableDefinitionExpression) {
  ALTACORE_MAKE_DH(VariableDefinitionExpression);
  return detail(info, noBody);
};

ALTACORE_AST_VALIDATE_D(VariableDefinitionExpression) {
  ALTACORE_VS_S(VariableDefinitionExpression);
  if (name.empty()) ALTACORE_VALIDATION_ERROR("empty name for variable definition");
  if (!type && !info->type) ALTACORE_VALIDATION_ERROR("empty type for variable definition");
  if (type) type->validate(stack, info->type);
  if (initializationExpression) {
    initializationExpression->validate(stack, info->initializationExpression);
  }
  for (auto& mod: modifiers) {
    if (mod.empty()) ALTACORE_VALIDATION_ERROR("empty modifer for variable definition");
  }
  if (
    // if it's not a native type
    !info->type->type->isNative &&
    // and the class has no default constructor
    !info->type->type->klass->defaultConstructor &&
    // and no initialization expression was provided
    !initializationExpression
  ) {
    // then there's a problem...
    ALTACORE_VALIDATION_ERROR("class has no default constructor; must be manually initialized");
  }
  if (info->type->isAny) {
    ALTACORE_VALIDATION_ERROR("variables can't have `any` type");
  }
  ALTACORE_VS_E;
};

ALTACORE_AST_INFO_DETAIL_D(VariableDefinitionExpression) {
  ALTACORE_CAST_DH(VariableDefinitionExpression);

  if (initializationExpression != nullptr && (!noBody || !type) && !info->initializationExpression) {
    info->initializationExpression = initializationExpression->fullDetail(info->inputScope);
  }

  if (!info->type) {
    if (!type && info->initializationExpression) {
      // specific check to help people remember you can't infer a type from `nullptr`
      if (std::dynamic_pointer_cast<AST::NullptrExpression>(info->initializationExpression)) {
        ALTACORE_DETAILING_ERROR("can't infer variable type from `nullptr` (i.e. type must be explicitly declared for this variable)");
      }
      info->type = std::make_shared<DH::Type>(info->inputScope);
      info->type->type = DET::Type::getUnderlyingType(info->initializationExpression.get())->deconstify();
      info->type->isAny = info->type->type->isAny;
      info->type->isNative = info->type->type->isNative;
      info->type->isFunction = info->type->type->isFunction;
      if (info->type->type->isFunction) {
        info->inputScope->hoist(info->type->type);
      } else if (!info->type->type->isNative && info->type->type->klass->genericParameterCount > 0) {
        info->inputScope->hoist(info->type->type->klass);
      }
    } else if (type) {
      info->type = type->fullDetail(info->inputScope);
    } else {
      ALTACORE_DETAILING_ERROR("variables must either have declared types or initializers (to infer their types)");
    }
  }

  if (!info->variable) {
    info->variable = std::make_shared<DET::Variable>(name, info->type->type, info->inputScope);
    info->inputScope->items.push_back(info->variable);
    info->variable->isLiteral = std::find(modifiers.begin(), modifiers.end(), "literal") != modifiers.end();
    info->variable->isExport = std::find(modifiers.begin(), modifiers.end(), "export") != modifiers.end();
    if (info->variable->isExport) {
      if (auto mod = Util::getModule(info->inputScope.get()).lock()) {
        mod->exports->items.push_back(info->variable);
      }
    }
  }

  return info;
};
