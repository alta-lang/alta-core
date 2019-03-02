#ifndef ALTACORE_OPTIONAL_HPP
#define ALTACORE_OPTIONAL_HPP

#if defined(__has_include) && __has_include(<optional>)
#include <optional>
#define ALTACORE_OPTIONAL std::optional
#define ALTACORE_NULLOPT std::nullopt
#else
#include <optional.hpp>
#define ALTACORE_OPTIONAL tl::optional
#define ALTACORE_NULLOPT tl::nullopt
#endif

#endif /* ALTACORE_OPTIONAL_HPP */
