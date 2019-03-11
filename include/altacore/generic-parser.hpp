#ifndef ALTACORE_GENERIC_PARSER_HPP
#define ALTACORE_GENERIC_PARSER_HPP

#include "parser.hpp"
#include <stack>

namespace AltaCore {
  namespace Parser {
    template<typename RT, typename TT> bool GenericExpectationType<RT, TT>::operator ==(const GenericExpectationType<RT, TT>& other) {
      if (valid != other.valid) return false;
      if (!valid) return true;

      if (isToken != other.isToken) return false;

      if (isToken && token != other.token) return false;
      if (!isToken && rule != other.rule) return false;

      return true;
    };
  };
};

#endif // ALTACORE_GENERIC_PARSER_HPP
