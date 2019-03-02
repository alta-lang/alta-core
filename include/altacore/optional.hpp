#ifndef ALTACORE_OPTIONAL_HPP
#define ALTACORE_OPTIONAL_HPP

#if defined(__has_include) && __has_include(<optional>)
#include <optional>
#define ALTACORE_OPTIONAL std::optional
#define ALTACORE_NULLOPT std::nullopt
#define ALTACORE_MAKE_OPTIONAL std::make_optional
#else
#include <optional.hpp>
#define ALTACORE_OPTIONAL tl::optional
#define ALTACORE_NULLOPT tl::nullopt
#define ALTACORE_MAKE_OPTIONAL tl::make_optional
#endif

#endif /* ALTACORE_OPTIONAL_HPP */
