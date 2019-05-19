#ifndef ALTACORE_VARIANT_HPP
#define ALTACORE_VARIANT_HPP

#if defined(__has_include) && __has_include(<variant>) && !defined(ALTACORE_NO_SYSTEM_VARIANT)
#include <variant>
#define ALTACORE_VARIANT std::variant
#define ALTACORE_VARIANT_HOLDS_ALTERNATIVE std::holds_alternative
#define ALTACORE_VARIANT_GET std::get
#define ALTACORE_VARIANT_GET_IF std::get_if
#else
#include <mpark/variant.hpp>
#define ALTACORE_VARIANT mpark::variant
#define ALTACORE_VARIANT_HOLDS_ALTERNATIVE mpark::holds_alternative
#define ALTACORE_VARIANT_GET mpark::get
#define ALTACORE_VARIANT_GET_IF mpark::get_if
#endif

#endif /* ALTACORE_VARIANT_HPP */
