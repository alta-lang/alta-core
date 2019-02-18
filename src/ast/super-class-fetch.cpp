#include "../../include/altacore/ast/super-class-fetch.hpp"
#include "../../include/altacore/ast/retrieval-node.hpp"
#include "../../include/altacore/ast/fetch.hpp"
#include "../../include/altacore/ast/accessor.hpp"
#include "../../include/altacore/ast/integer-literal-node.hpp"
#include "../../include/altacore/ast/function-call-expression.hpp"
#include "../../include/altacore/util.hpp"

const AltaCore::AST::NodeType AltaCore::AST::SuperClassFetch::nodeType() {
  return NodeType::SuperClassFetch;
};

ALTACORE_AST_DETAIL_D(SuperClassFetch) {
  ALTACORE_MAKE_DH(SuperClassFetch);

  if (!fetch) {
    fetch = std::make_shared<IntegerLiteralNode>("0");
  }

  info->klass = Util::getClass(scope).lock();
  if (!info->klass) {
    ALTACORE_DETAILING_ERROR("superclass fetches can only appear in classes");
  }

  if (auto intLit = std::dynamic_pointer_cast<AST::IntegerLiteralNode>(fetch)) {
    auto num = std::stoull(intLit->raw);
    if (info->klass->parents.size() < num + 1) {
      ALTACORE_DETAILING_ERROR("class does not have enough superclasses");
    }
    info->parentClassIndex = num;
    info->superclass = info->klass->parents[num];
  } else if (auto ret = std::dynamic_pointer_cast<AST::RetrievalNode>(fetch)) {
    auto det = ret->fullDetail(scope);
    if (det->items.size() > 1) {
      ALTACORE_DETAILING_ERROR("multiple classes found that match the query");
    }
    if (det->items.size() < 1) {
      ALTACORE_DETAILING_ERROR("no classes found that match the query");
    }
    if (det->items[0]->nodeType() != DET::NodeType::Class) {
      ALTACORE_DETAILING_ERROR("query did not return a class");
    }
    auto& query = det->items[0];
    for (size_t i = 0; i < info->klass->parents.size(); i++) {
      auto& parent = info->klass->parents[i];
      if (query->id == parent->id) {
        info->parentClassIndex = i;
        info->superclass = parent;
        break;
      }
    }

    if (!info->superclass) {
      ALTACORE_DETAILING_ERROR("queried class is not a superclass of current class");
    }
  } else {
    ALTACORE_DETAILING_ERROR("invalid superclass fetch target");
  }

  std::vector<std::shared_ptr<DET::Type>> targetTypes;
  std::unordered_map<size_t, size_t> indexMap;
  for (size_t i = 0; i < info->superclass->constructors.size(); i++) {
    auto& constr = info->superclass->constructors[i];
    if (!scope->canSee(constr)) {
      continue;
    }
    indexMap[targetTypes.size()] = i;
    targetTypes.push_back(std::make_shared<DET::Type>(constr->returnType, constr->parameters));
  }

  std::vector<std::tuple<std::string, std::shared_ptr<ExpressionNode>, std::shared_ptr<DH::ExpressionNode>>> argsWithDet;

  for (auto& [name, arg]: arguments) {
    auto det = arg->fullDetail(scope);
    argsWithDet.emplace_back(name, arg, det);
    info->arguments.push_back(det);
  }

  auto [index, argMap, adjArgs] = FunctionCallExpression::findCompatibleCall(argsWithDet, targetTypes);

  if (index != SIZE_MAX) {
    info->constructor = info->superclass->constructors[indexMap[index]];
    info->adjustedArguments = adjArgs;
    info->argumentMap = argMap;
  } else {
    ALTACORE_DETAILING_ERROR("unable to find suitable constructor");
  }

  return info;
};

ALTACORE_AST_VALIDATE_D(SuperClassFetch) {
  ALTACORE_VS_S(SuperClassFetch);
  
  ALTACORE_VS_E;
};
