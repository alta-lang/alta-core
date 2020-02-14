#include "../../include/altacore/ast/special-fetch-expression.hpp"

const AltaCore::AST::NodeType AltaCore::AST::SpecialFetchExpression::nodeType() {
  return NodeType::SpecialFetchExpression;
};

ALTACORE_AST_DETAIL_D(SpecialFetchExpression) {
  ALTACORE_MAKE_DH(SpecialFetchExpression);

  if (attributes.size() == 1) {
    info->attributes = Attributes::detailAttributes(attributes, info->inputScope, shared_from_this(), info);
  } else {
    info->items = info->inputScope->findAll(query, {}, true, info->inputScope);

    if (info->items.size() < 1) {
      ALTACORE_DETAILING_ERROR("invalid special fetch or special fetch used in invalid location (no fetch targets found)");
    } else if (info->items.size() > 1) {
      ALTACORE_DETAILING_ERROR("impossible special fetch error encountered (multiple fetch targets found)");
    }
  }

  return info;
};

ALTACORE_AST_VALIDATE_D(SpecialFetchExpression) {
  ALTACORE_VS_S(SpecialFetchExpression);
  ALTACORE_VS_E;
};
