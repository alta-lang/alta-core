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

    template<typename RT> bool GenericState<RT>::operator ==(const GenericState<RT>& rhs) const {
      if (currentPosition != rhs.currentPosition) return false;
      return true;
    };

    template<typename RT, typename TT, class T> auto GenericParser<RT, TT, T>::expect(std::vector<typename GenericParser<RT, TT, T>::ExpectationType> expectations) -> Expectation {
      Expectation ret; // by default, Expectations are invalid
      const auto stateAtStart = currentState;
      State state = stateAtStart;

#ifdef ALTACORE_GENERIC_PARSER_USE_FAILED
      if (failed.find(stateAtStart.currentPosition) == failed.end()) {
        failed[stateAtStart.currentPosition] = std::unordered_set<RT>();
      }
#endif

      if (state.currentPosition >= tokens.size()) return ret;

      for (auto& expectation: expectations) {
        state = stateAtStart;
        currentState = state;
        if (!expectation.isToken) {
          std::stack<std::tuple<RT, std::stack<ExpectationType>, RuleState, std::vector<Expectation>, State>> ruleStack;
          ruleStack.emplace(
            expectation.rule,
            std::stack<ExpectationType>(),
            RuleState(stateAtStart),
            std::vector<Expectation>(),
            stateAtStart
          );

          auto tmp = runRule(expectation.rule, std::get<2>(ruleStack.top()), std::get<3>(ruleStack.top()));
          ALTACORE_OPTIONAL<T> finalVal;

          while (ruleStack.size() > 0) {
            int i = 3;
            ExpectationType nextRule = std::get<0>(ruleStack.top());

            // whether to ignore the rulesToIgnore and currentFails
            // this is used to return to rules that previously yielded
#ifdef ALTACORE_GENERIC_PARSER_USE_FAILED
            bool ignoreIgnore = false;
#endif

            auto position = std::get<4>(ruleStack.top()).currentPosition;
#ifdef ALTACORE_GENERIC_PARSER_USE_FAILED
            if (failed.find(position) == failed.end()) {
              failed[position] = std::unordered_set<RT>();
            }
            auto& currentFails = failed[position];
#endif
            if (!ALTACORE_VARIANT_HOLDS_ALTERNATIVE<ALTACORE_OPTIONAL<T>>(tmp)) {
              auto& [ruleType, expTypes, ruleState, exps, stateAtStart] = ruleStack.top();
              expTypes = std::stack<ExpectationType>();
              if (ALTACORE_VARIANT_HOLDS_ALTERNATIVE<ExpectationType>(tmp)) {
                expTypes.push(ALTACORE_VARIANT_GET<ExpectationType>(tmp));
              } else {
                auto& list = ALTACORE_VARIANT_GET<std::initializer_list<ExpectationType>>(tmp);
                for (auto it = std::rbegin(list); it != std::rend(list); ++it) {
                  expTypes.push(*it);
                }
              }
              if (expTypes.top().isToken) {
                throw std::runtime_error("invalid yielded expectation in coroutine rule. NOTE: to use token type expectations, use `expect` instead of yielding");
              }
              ruleStack.emplace(
                expTypes.top().rule,
                std::stack<ExpectationType>(),
                RuleState(currentState),
                std::vector<Expectation>(),
                currentState
              );
              nextRule = expTypes.top();
            } else {
              finalVal = ALTACORE_VARIANT_GET<ALTACORE_OPTIONAL<T>>(tmp);
              auto rule = std::get<0>(ruleStack.top());
              auto stateAtStart = std::get<4>(ruleStack.top());
              ruleStack.pop();
              if (ruleStack.size() == 0) {
                continue;
              }
              auto& [ruleType, expTypes, ruleState, exps, _ignored1] = ruleStack.top();
              expTypes.pop();
              Expectation exp;
              if (finalVal) {
#ifdef ALTACORE_GENERIC_PARSER_USE_FAILED
                currentFails.erase(rule);
#endif
                exp.valid = true;
                exp.type = rule;
                exp.item = finalVal;
                state = currentState;

                exps.push_back(exp);
                ruleState.iteration++;
                nextRule = ruleType;
#ifdef ALTACORE_GENERIC_PARSER_USE_FAILED
                ignoreIgnore = true;
#endif
              } else {
#ifdef ALTACORE_GENERIC_PARSER_USE_FAILED
                currentFails.insert(rule);
#endif
                currentState = stateAtStart;
                if (expTypes.size() == 0) {
                  exps.push_back(exp);
                  ruleState.iteration++;
                  nextRule = ruleType;
#ifdef ALTACORE_GENERIC_PARSER_USE_FAILED
                  ignoreIgnore = true;
#endif
                } else {
                  ruleStack.emplace(
                    expTypes.top().rule,
                    std::stack<ExpectationType>(),
                    RuleState(currentState),
                    std::vector<Expectation>(),
                    currentState
                  );
                  nextRule = expTypes.top();
                }
              }
            }
            if (nextRule.isToken) {
              throw std::runtime_error("invalid yielded expectation in coroutine rule. NOTE: to use token type expectations, use `expect` instead of yielding");
            }
            auto& [_ignored1, _ignored2, newRuleState, newRuleExps, newState] = ruleStack.top();
#ifdef ALTACORE_GENERIC_PARSER_USE_FAILED
            auto& newCurrentFails = failed[newState.currentPosition];
            if (!ignoreIgnore && newCurrentFails.find(nextRule.rule) != newCurrentFails.end()) {
              tmp = ALTACORE_NULLOPT;
              continue;
            }
#endif
            tmp = runRule(nextRule.rule, newRuleState, newRuleExps);
            newRuleState.currentState = currentState;
            if (newRuleState.currentState.currentPosition > farthestRule.currentState.currentPosition) {
              farthestRule = newRuleState;
            }
          }

          if (finalVal) {
            ret.valid = true;
            ret.type = expectation.rule;
            ret.item = finalVal;
            state = currentState;
          } else {
#ifdef ALTACORE_GENERIC_PARSER_USE_FAILED
            failed[stateAtStart.currentPosition].insert(expectation.rule);
#endif
          }
        } else {
          if (tokens[state.currentPosition].type == expectation.token) {
            ret.valid = true;
            ret.type = expectation.token;
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
