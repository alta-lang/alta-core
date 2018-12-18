#ifndef ALTACORE_GENERIC_PARSER_HPP
#define ALTACORE_GENERIC_PARSER_HPP

#include "parser.hpp"
#include <stack>

namespace AltaCore {
  namespace Parser {
    template<typename RT> bool GenericState<RT>::operator ==(const GenericState<RT>& rhs) const {
      if (currentPosition != rhs.currentPosition) return false;
      if (rulesToIgnore.size() != rhs.rulesToIgnore.size()) return false;
      for (size_t i = 0; i< rulesToIgnore.size(); i++) {
        if (rulesToIgnore[i] != rhs.rulesToIgnore[i]) {
          return false;
        }
      }
      return true;
    };

    template<typename RT, typename TT, class T> auto GenericParser<RT, TT, T>::expect(std::initializer_list<typename GenericParser<RT, TT, T>::ExpectationType> expectations) -> Expectation {
      Expectation ret; // by default, Expectations are invalid
      const auto stateAtStart = currentState;
      State state = stateAtStart;

      if (failed.find(stateAtStart.currentPosition) == failed.end()) {
        failed[stateAtStart.currentPosition] = std::unordered_set<RT>();
      }

      if (state.currentPosition >= tokens.size()) return ret;

      for (auto& expectation: expectations) {
        state = stateAtStart;
        currentState = state;
        /*
        if (!expectation.isToken) {
          if (std::find(rulesToIgnore.begin(), rulesToIgnore.end(), expectation.rule) != rulesToIgnore.end()) {
            continue;
          }
          if (std::find(currentFails.begin(), currentFails.end(), expectation.rule) != currentFails.end()) {
            continue;
          }
          if (loopCache.find(expectation.rule) != loopCache.end() && loopCache[expectation.rule] == state) {
            continue;
          }
        }
        */
        if (!expectation.isToken) {
          //loopCache[expectation.rule] = state;
          /*
          auto& table = ruleTable();
          if (!table[expectation.rule]) {
            throw std::runtime_error("no handler is registered for the given rule");
          }
          */
          std::stack<std::tuple<RT, std::stack<ExpectationType>, RuleState, std::vector<Expectation>, State>> ruleStack;
          ruleStack.emplace(
            expectation.rule,
            std::stack<ExpectationType>(),
            RuleState(),
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
            bool ignoreIgnore = false;
            
            auto position = std::get<4>(ruleStack.top()).currentPosition;
            if (failed.find(position) == failed.end()) {
              failed[position] = std::unordered_set<RT>();
            }
            auto& currentFails = failed[position];
            if (!std::holds_alternative<ALTACORE_OPTIONAL<T>>(tmp)) {
              auto& [ruleType, expTypes, ruleState, exps, stateAtStart] = ruleStack.top();
              expTypes = std::stack<ExpectationType>();
              if (std::holds_alternative<ExpectationType>(tmp)) {
                expTypes.push(std::get<ExpectationType>(tmp));
              } else {
                auto& list = std::get<std::initializer_list<ExpectationType>>(tmp);
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
                RuleState(),
                std::vector<Expectation>(),
                currentState
              );
              nextRule = expTypes.top();
            } else {
              finalVal = std::get<ALTACORE_OPTIONAL<T>>(tmp);
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
                currentFails.erase(rule);
                exp.valid = true;
                exp.type.isToken = false;
                exp.type.rule = rule;
                exp.item = finalVal;
                state = currentState;

                exps.push_back(exp);
                ruleState.iteration++;
                nextRule = ruleType;
                ignoreIgnore = true;
              } else {
                currentFails.insert(rule);
                currentState = stateAtStart;
                if (expTypes.size() == 0) {
                  exps.push_back(exp);
                  ruleState.iteration++;
                  nextRule = ruleType;
                  ignoreIgnore = true;
                } else {
                  ruleStack.emplace(
                    expTypes.top().rule,
                    std::stack<ExpectationType>(),
                    RuleState(),
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
            auto& newCurrentFails = failed[newState.currentPosition];
            if (!ignoreIgnore && std::find(rulesToIgnore.begin(), rulesToIgnore.end(), nextRule.rule) != rulesToIgnore.end()) {
              tmp = ALTACORE_NULLOPT;
              continue;
            }
            if (!ignoreIgnore && newCurrentFails.find(nextRule.rule) != newCurrentFails.end()) {
              tmp = ALTACORE_NULLOPT;
              continue;
            }
            tmp = runRule(nextRule.rule, newRuleState, newRuleExps);
          }

          if (finalVal) {
            ret.valid = true;
            ret.type.isToken = false;
            ret.type.rule = expectation.rule;
            ret.item = finalVal;
            state = currentState;
          } else {
            failed[stateAtStart.currentPosition].insert(expectation.rule);
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
