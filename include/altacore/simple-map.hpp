#ifndef ALTACORE_SIMPLE_MAP_HPP
#define ALTACORE_SIMPLE_MAP_HPP

#if defined(__has_include) && __has_include(<unordered_map>)
#include <unordered_map>
#define ALTACORE_MAP std::unordered_map
#else
#include <map>
#define ALTACORE_MAP std::map
#endif

#endif /* ALTACORE_SIMPLE_MAP_HPP */
