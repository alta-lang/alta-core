#ifndef ALTACORE_ALTACORE_HPP
#define ALTACORE_ALTACORE_HPP

#include "altacore/lexer.hpp"
#include "altacore/parser.hpp"
#include "altacore/ast.hpp"
#include "altacore/det.hpp"
#include "altacore/modules.hpp"
#include "altacore/util.hpp"
#include "altacore/preprocessor.hpp"
#include "altacore/attributes.hpp"
#include "altacore/validator.hpp"

namespace AltaCore {
  void registerGlobalAttributes();
};

#endif // ALTACORE_ALTACORE_HPP
