#include "../../include/altacore/ast/variable-definition-expression.hpp"
#include "../../include/altacore/util.hpp"
#include "../../include/altacore/ast/nullptr-expression.hpp"
#include "../../include/altacore/logging.hpp"

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
    // if no initialization expression was provided
    !initializationExpression &&
    // and it's a class or union type
    !info->type->type->isNative &&
    (
      info->type->type->isUnion() ||
      (
        info->type->type->klass &&
        // and the class has no default constructor
        !info->type->type->klass->defaultConstructor
      )
    )
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

  if (noBody) {
    if (!info->type && type) {
      info->type = type->fullDetail(info->inputScope);
    }
    if (!info->variable) {
      info->variable = std::make_shared<DET::Variable>(name, info->type ? info->type->type : nullptr, position, info->inputScope);
      info->inputScope->items.push_back(info->variable);
      info->variable->isLiteral = std::find(modifiers.begin(), modifiers.end(), "literal") != modifiers.end();
      info->variable->isExport = std::find(modifiers.begin(), modifiers.end(), "export") != modifiers.end();
      if (info->variable->isExport) {
        if (auto mod = Util::getModule(info->inputScope.get()).lock()) {
          mod->exports->items.push_back(info->variable);
        }
      }
    }
  } else {
    if (initializationExpression != nullptr && !info->initializationExpression) {
      info->initializationExpression = initializationExpression->fullDetail(info->inputScope);
    }
    if (!info->type) {
      if (!type && info->initializationExpression) {
        // specific check to help people remember you can't infer a type from `nullptr`
        if (std::dynamic_pointer_cast<AST::NullptrExpression>(info->initializationExpression)) {
          ALTACORE_DETAILING_ERROR("can't infer variable type from `nullptr` (i.e. type must be explicitly declared for this variable)");
        }
        auto initType = DET::Type::getUnderlyingType(info->initializationExpression.get())->deconstify();
        auto inferred = initType->destroyReferences();
        if (
          // warn for types with references...
          initType->referenceLevel() > 0 &&
          // ...but NOT for retrieval nodes with a single reference
          // (since they simply reference the variable directly and the actual behavior is the expected behavior)
          // NOTE: this won't catch instances where a retrieval node returns a reference variable
          !(std::dynamic_pointer_cast<RetrievalNode>(initializationExpression) && initType->referenceLevel() == 1)
        ) {
          Logging::log(Logging::Message("AST", "T0001", Logging::Severity::Warning, position, "Inferred variable type without references:\n  Initialization expression type = " + initType->toString() + "\n  Inferred variable type = " + inferred->toString()));
        }
        info->type = std::make_shared<DH::Type>(info->inputScope);
        info->type->type = inferred;
        info->type->isAny = info->type->type->isAny;
        info->type->isNative = info->type->type->isNative;
        info->type->isFunction = info->type->type->isFunction;
        info->inputScope->hoist(info->type->type);
      } else if (type) {
        info->type = type->fullDetail(info->inputScope);
      } else {
        ALTACORE_DETAILING_ERROR("variables must either have declared types or initializers (to infer their types)");
      }
    }
    if (info->inputScope.get() != info->type->inputScope.get()) {
      info->inputScope->hoist(info->type->type);
    }
    if (!info->variable) {
      info->variable = std::make_shared<DET::Variable>(name, info->type->type, position, info->inputScope);
      info->inputScope->items.push_back(info->variable);
      info->variable->isLiteral = std::find(modifiers.begin(), modifiers.end(), "literal") != modifiers.end();
      info->variable->isExport = std::find(modifiers.begin(), modifiers.end(), "export") != modifiers.end();
      if (info->variable->isExport) {
        if (auto mod = Util::getModule(info->inputScope.get()).lock()) {
          mod->exports->items.push_back(info->variable);
        }
      }
    }
    if (!info->variable->type) {
      info->variable->type = info->type->type;
    }
  }

  return info;
};
