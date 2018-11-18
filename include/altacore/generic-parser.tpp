#ifndef ALTACORE_GENERIC_PARSER_HPP
#define ALTACORE_GENERIC_PARSER_HPP

#include "parser.hpp"

namespace AltaCore {
  namespace Parser {
    template<typename RT, typename TT, class T> auto GenericParser<RT, TT, T>::expect(std::initializer_list<typename GenericParser<RT, TT, T>::ExpectationType> expectations) -> Expectation {
      Expectation ret; // by default, Expectations are invalid
      const State stateAtStart = currentState;
      State state = stateAtStart;

      if (failed.find(stateAtStart.currentPosition) == failed.end()) {
        failed[stateAtStart.currentPosition] = std::vector<RT>();
      }

      if (state.currentPosition >= tokens.size()) return ret;

      for (auto& expectation: expectations) {
        auto& currentFails = failed[stateAtStart.currentPosition];
        state = stateAtStart;
        currentState = state;
        if (!expectation.isToken) {
          if (std::find(rulesToIgnore.begin(), rulesToIgnore.end(), expectation.rule) != rulesToIgnore.end()) {
            continue;
          }
          if (std::find(currentFails.begin(), currentFails.end(), expectation.rule) != currentFails.end()) {
            continue;
          }
        }
        if (!expectation.isToken) {
          auto tmp = runRule(expectation.rule);
          if (tmp.has_value()) {
            ret.valid = true;
            ret.type.isToken = false;
            ret.type.rule = expectation.rule;
            ret.item = tmp;
            state = currentState;
            break;
          } else {
            currentFails.push_back(expectation.rule);
          }
        } else {
          if (tokens[state.currentPosition].type == expectation.token) {
            ret.valid = true;
            ret.type.isToken = true;
            ret.type.token = expectation.token;
            ret.token = tokens[state.currentPosition];
            state.currentPosition++;
            break;
          } else {
            // TODO - for now, checking every time is fine since
            // the test for the token is a simple index lookup and
            // equality comparison (doesn't really take that much
            // processing time/power)
          }
        }
      }

      if (ret.valid) {
        currentState = state;
      } else {
        currentState = stateAtStart;
      }

      return ret;
    };
    template<typename RT, typename TT, class T> auto GenericParser<RT, TT, T>::expectAnyToken() -> Expectation {
      Expectation ret;

      if (tokens.size() > currentState.currentPosition) {
        ret.valid = true;
        ret.token = tokens[currentState.currentPosition];
        ret.type.isToken = true;
        ret.type.token = ret.token.type;
        currentState.currentPosition++;
      }

      return ret;
    };
  };
};

#endif // ALTACORE_GENERIC_PARSER_HPP
