#ifndef ALTACORE_ANY_HPP
#define ALTACORE_ANY_HPP

#if defined(__has_include) && __has_include(<any>)
#include <any>
#define ALTACORE_ANY std::any
#define ALTACORE_ANY_CAST std::any_cast
#define ALTACORE_ANY_HAS_VALUE(x) x.has_value()
#else
#include "../../deps/any/any.hpp"
#define ALTACORE_ANY linb::any
#define ALTACORE_ANY_CAST linb::any_cast
#define ALTACORE_ANY_HAS_VALUE(x) !x.empty()
#endif

#endif /* ALTACORE_ANY_HPP */
