#include "../../include/altacore/ast/generic.hpp"
#include "../../include/altacore/det/alias.hpp"
#include "../../include/altacore/det/type.hpp"

const AltaCore::AST::NodeType AltaCore::AST::Generic::nodeType() {
  return NodeType::Generic;
};

ALTACORE_AST_DETAIL_D(Generic) {
  ALTACORE_MAKE_DH(Generic);

  // TL;DR: we use void to make possible errors easier to detect
  //
  // void type as default is as close to null as we can get
  //
  // it is unacceptable for most operations, thus leading to runtime
  // errors and making it easier to detect accidental leakage of default
  // generic type alias values
  info->alias = std::make_shared<DET::Alias>(name, std::make_shared<DET::Type>(DET::NativeType::Void), info->inputScope);
  info->inputScope->items.push_back(info->alias);

  return info;
};
