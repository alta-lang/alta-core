#include "../../include/altacore/ast/lambda-expression.hpp"
#include "../../include/altacore/util.hpp"

const AltaCore::AST::NodeType AltaCore::AST::LambdaExpression::nodeType() {
  return NodeType::LambdaExpression;
};

ALTACORE_AST_DETAIL_D(LambdaExpression) {
  ALTACORE_MAKE_DH(LambdaExpression);

  info->function = DET::Function::create(info->inputScope, "@lambda@", {}, nullptr);

  info->function->isLambda = true;

  info->function->isLiteral = false;
  info->function->isExport = false;

  std::vector<std::tuple<std::string, std::shared_ptr<DET::Type>, bool, std::string>> params;

  for (auto& param: parameters) {
    auto det = param->fullDetail(info->function->scope, false);
    info->parameters.push_back(det);
    params.push_back(std::make_tuple(param->name, det->type->type, param->isVariable, param->id));
  }

  info->returnType = returnType->fullDetail(info->function->scope, false);
  Util::exportClassIfNecessary(info->function->scope, info->returnType->type);

  info->function->recreate(params, info->returnType->type);

  for (auto& attr: attributes) {
    info->attributes.push_back(attr->fullDetail(info->inputScope, shared_from_this(), info));
  }

  info->function->beganThrowing.listen([=]() {
    info->function->scope->isTry = true;
  });

  info->body = body->fullDetail(info->function->scope);

  info->toReference = info->function->referencedVariables;

  for (size_t i = 0; i < info->toReference.size(); i++) {
    bool remove = false;
    for (auto& copyVar: info->toCopy) {
      if (copyVar->id == info->toReference[i]->id) {
        remove = true;
        break;
      }
    }
    if (remove) {
      info->toReference.erase(info->toReference.begin() + i);
      --i;
    }
  }

  for (size_t i = 0; i < info->toCopy.size(); ++i) {
    bool found = false;
    for (auto& item: info->function->privateHoistedItems) {
      if (item->id == info->toCopy[i]->id) {
        found = true;
        break;
      }
    }
    if (!found) {
      info->toCopy.erase(info->toCopy.begin() + i);
      --i;
    }
  }

  info->function->doneDetailing.dispatch();

  return info;
};

ALTACORE_AST_VALIDATE_D(LambdaExpression) {
  ALTACORE_VS_S(LambdaExpression);
  ALTACORE_VS_E;
};
