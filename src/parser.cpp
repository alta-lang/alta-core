#include <algorithm>
#include "../include/altacore/parser.hpp"
#include "../include/altacore/util.hpp"

namespace AltaCore {
  namespace Parser {
    #define ACP_NOT_OK { next(); continue; }
    #define ACP_NODE(x) { next(true, {}, x); continue; }
    #define ACP_RULES(x) { next(true, x); continue; }
    #define ACP_RULE_LIST(...) { next(true, { __VA_ARGS__ }); continue; }
    #define ACP_EXP(x) { if (x) { next(true, {}, *x); } else { next(false); }; continue; }
    #define ACP_RULE(x) { next(true, { RuleType::x }); continue; }

    Token Parser::expect(std::vector<TokenType> expectations, bool rawPrepo) {
      Token tok;
      tok.valid = false;

      if (currentState.currentPosition >= tokens.size()) return tok;

      if (!rawPrepo && tokens[currentState.currentPosition].type == TokenType::PreprocessorSubstitution) {
        auto& curr = tokens[currentState.currentPosition];
        relexer.tokens = tokens;
        auto name = curr.raw.substr(2, curr.raw.size() - 3);
        auto def = (definitions.find(name) != definitions.end()) ? definitions[name] : PrepoExpression();
        Timing::parseTimes[filePath].stop();
        relexer.relex(currentState.currentPosition, def);
        Timing::parseTimes[filePath].start();
        tokens = relexer.tokens;
      }

      for (auto& expectation: expectations) {
        if (tokens[currentState.currentPosition].type == expectation) {
          tok = tokens[currentState.currentPosition++];
          break;
        }
      }

      return tok;
    };

    Token Parser::peek(size_t lookahead, bool lookbehind) {
      Token tok;
      tok.valid = false;

      auto position = (lookbehind) ? (currentState.currentPosition - lookahead) : (currentState.currentPosition + lookahead);

      if (position >= tokens.size()) return tok;

      return tokens[position];
    };

    Token Parser::expectAnyToken() {
      if (tokens.size() > currentState.currentPosition) {
        return tokens[currentState.currentPosition++];
      }

      Token tok;
      tok.valid = false;
      return tok;
    };

    bool State::operator ==(const State& rhs) const {
      if (currentPosition != rhs.currentPosition) return false;
      return true;
    };

    bool PrepoExpression::operator ==(const PrepoExpression& right) {
      if (type != right.type) return false;
      if (type == PrepoExpressionType::Boolean) {
        return boolean == right.boolean;
      } else if (type == PrepoExpressionType::String) {
        return string == right.string;
      } else {
        // both are null or undefined
        return true;
      }
    };

    PrepoExpression::operator bool() {
      if (type == PrepoExpressionType::Boolean) {
        return boolean;
      } else if (type == PrepoExpressionType::String) {
        return !string.empty();
      } else {
        // null and undefined are non-truthy
        return false;
      }
    };

    PrepoExpression::operator std::string() {
      if (type == PrepoExpressionType::Boolean) {
        return (boolean) ? "true" : "false";
      } else if (type == PrepoExpressionType::String) {
        return string;
      } else {
        // null and undefined produce empty strings
        return "";
      }
    };

    // <rule-state-structures>
    template<typename S> struct ConditionStatementState {
      S state;
      std::shared_ptr<AST::ConditionalStatement> cond = nullptr;

      ConditionStatementState(S _state, decltype(cond) _cond):
        state(_state),
        cond(_cond)
        {};
    };
    template<typename S> struct VerbalConditionalState {
      std::shared_ptr<AST::ConditionalExpression> cond = nullptr;
      bool isRepeat = false;
      S stateCache;

      VerbalConditionalState(decltype(cond) _cond, S _stateCache, bool _isRepeat = false):
        cond(_cond),
        stateCache(_stateCache),
        isRepeat(_isRepeat)
        {};
    };
    // </rule-state-structures>

    // <helper-functions>
    ALTACORE_OPTIONAL<std::string> Parser::expectModifier(ModifierTargetType mtt) {
      auto state = currentState;
      if (auto mod = expect(TokenType::Identifier)) {
        for (auto& modifier: modifiersForTargets[(unsigned int)mtt]) {
          if (mod.raw == modifier) {
            return mod.raw;
          }
        }
      }
      currentState = state;
      return ALTACORE_NULLOPT;
    };
    std::vector<std::string> Parser::expectModifiers(ModifierTargetType mtt) {
      std::vector<std::string> modifiers;
      Token mod;
      State state;
      while ((state = currentState), (mod = expect(TokenType::Identifier)), mod.valid) {
        bool cont = false;
        for (auto& modifier: modifiersForTargets[(unsigned int)mtt]) {
          if (mod.raw == modifier) {
            modifiers.push_back(mod.raw);
            cont = true;
            break;
          }
        }
        if (cont) continue;
        currentState = state;
        break;
      }
      return modifiers;
    };
    bool Parser::expectKeyword(std::string keyword) {
      auto state = currentState;
      auto exp = expect(TokenType::Identifier);
      if (exp && exp.raw == keyword) return true;
      currentState = state;
      return false;
    };

    bool Parser::expectBinaryOperation(RuleType rule, RuleType nextHigherPrecedentRule, std::vector<TokenType> operatorTokens, std::vector<AST::OperatorType> operatorTypes, RuleState& state, std::vector<Expectation>& exps, std::shared_ptr<AST::Node>& ruleNode, NextFunctionType next, SaveStateType saveState, RestoreStateType restoreState) {
      if (operatorTokens.size() != operatorTypes.size()) {
        throw std::runtime_error("malformed binary operation expectation: the number of operator tokens must match the number of operator types.");
      }
      if (state.internalIndex == 0) {
        state.internalIndex = 1;
        next(true, { nextHigherPrecedentRule }, nullptr);
        return true;
      } else if (state.internalIndex == 1) {
        if (!exps.back()) {
          next(false, {}, nullptr);
          return true;
        }

        auto binOp = std::make_shared<AST::BinaryOperation>();
        binOp->left = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);

        auto opExp = expect(operatorTokens);
        if (!opExp) {
          if (exps.back().item) {
            next(true, {}, *exps.back().item);
          } else {
            next(false, {}, nullptr);
          }
          return true;
        }

        // comment 0.0:
        // must be initialized to keep the compiler happy
        AST::OperatorType op = AST::OperatorType::Addition;
        for (size_t i = 0; i < operatorTokens.size(); i++) {
          if (opExp.type == operatorTokens[i]) {
            op = operatorTypes[i];
            break;
          }
        }
        binOp->type = op;

        saveState();
        ruleNode = std::move(binOp);
        state.internalIndex = 2;

        next(true, { nextHigherPrecedentRule }, nullptr);
        return true;
      } else {
        auto binOp = std::dynamic_pointer_cast<AST::BinaryOperation>(ruleNode);

        if (!exps.back()) {
          if (state.internalIndex == 2) {
            next(false, {}, nullptr);
            return true;
          } else {
            restoreState();
            next(true, {}, binOp->left);
            return true;
          }
        }

        binOp->right = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);

        Token opExp = expect(operatorTokens);

        if (opExp) {
          // see comment 0.0
          AST::OperatorType op = AST::OperatorType::Addition;
          for (size_t i = 0; i < operatorTokens.size(); i++) {
            if (opExp.type == operatorTokens[i]) {
              op = operatorTypes[i];
              break;
            }
          }

          auto otherBinOp = std::make_shared<AST::BinaryOperation>();
          otherBinOp->left = binOp;
          otherBinOp->type = op;

          saveState();
          ruleNode = std::move(otherBinOp);
          state.internalIndex = 3;
          next(true, { nextHigherPrecedentRule }, nullptr);
          return true;
        } else {
          next(true, {}, binOp);
          return true;
        }
      }
      return true;
    };
    // </helper-functions>

    PrepoExpression Prepo::defined(std::vector<PrepoExpression> targets) {
      for (auto& target: targets) {
        if (target.type == PrepoExpressionType::Undefined) {
          return false;
        }
      }
      return true;
    };

    ALTACORE_OPTIONAL<PrepoExpression> Parser::expectPrepoExpression() {
      std::stack<PrepoRuleStackElement> ruleStack;
      ALTACORE_OPTIONAL<PrepoExpression> root = ALTACORE_NULLOPT;

      ruleStack.emplace(
        PrepoRuleType::Root,
        std::stack<PrepoRuleType>(),
        RuleState(currentState),
        std::vector<PrepoExpectation>(),
        PrepoExpression(),
        currentState
      );

      auto next = [&](bool ok = false, std::vector<PrepoRuleType> rules = {}, PrepoExpression result = PrepoExpression()) {
        auto& state = std::get<2>(ruleStack.top());
        state.iteration++;
        
        auto& ruleExps = std::get<1>(ruleStack.top());
        for (auto it = rules.rbegin(); it != rules.rend(); it++) {
          ruleExps.push(*it);
        }

        if (ok && rules.size() > 0) return;

        auto oldRuleType = std::get<0>(ruleStack.top());
        auto oldState = std::get<2>(ruleStack.top());
        ruleStack.pop();

        if (ruleStack.size() < 1) return;

        auto& [newRule, newNextExps, newRuleState, newExps, ruleNode, stateCache] = ruleStack.top();

        if (!ok) {
          currentState = oldState.stateAtStart;
          if (newNextExps.size() < 1) {
            newExps.push_back(PrepoExpectation()); // push back an invalid PrepoExpectation
          }
        } else if (ruleExps.size() == 0) {
          newNextExps = {};
          newExps.push_back(PrepoExpectation(oldRuleType, result));
        }
      };

      auto saveState = [&]() {
        if (ruleStack.size() < 1) return;
        std::get<5>(ruleStack.top()) = currentState;
      };
      auto restoreState = [&]() {
        if (ruleStack.size() < 1) return;
        currentState = std::get<5>(ruleStack.top());
      };

      auto currentLine = peek(1, true).line;

      auto expect = [&](TokenType type) -> Token {
        auto invalidToken = Token();
        invalidToken.valid = false;
        if (peek().line != currentLine) return invalidToken;
        return this->expect({ type }, true);
      };

      while (ruleStack.size() > 0) {
        auto& [rule, nextExps, state, exps, ruleNode, stateCache] = ruleStack.top();

        if (nextExps.size() > 0) {
          auto nextExp = nextExps.top();
          nextExps.pop();

          ruleStack.emplace(
            nextExp,
            std::stack<PrepoRuleType>(),
            RuleState(currentState),
            std::vector<PrepoExpectation>(),
            PrepoExpression(),
            currentState
          );
          continue;
        }

        #define PREPO_NOT_OK { next(); continue; }
        #define PREPO_NODE(x) { next(true, {}, x); continue; }
        #define PREPO_RULES(x) { next(true, x); continue; }
        #define PREPO_RULE_LIST(...) { next(true, { __VA_ARGS__ }); continue; }
        #define PREPO_EXP(x) { if (x) { next(true, {}, *x); } else { next(false); }; continue; }
        #define PREPO_RULE(x) { next(true, { PrepoRuleType::x }); continue; }

        if (rule == PrepoRuleType::Root) {
          if (state.iteration == 0) PREPO_RULE(Expression);
          if (!exps.back()) PREPO_NOT_OK;
          root = exps.back().item;
          
          next(true);
          break;
        } else if (rule == PrepoRuleType::Expression) {
          if (state.iteration == 0) PREPO_RULE(Or);
          if (!exps.back()) PREPO_NOT_OK;
          PREPO_EXP(exps.back().item);
        } else if (rule == PrepoRuleType::Equality) {
          if (state.internalIndex == 0) {
            state.internalIndex = 1;
            PREPO_RULE(MacroCall);
          } else if (state.internalIndex == 1) {
            if (!exps.back()) PREPO_NOT_OK;

            if (!expect(TokenType::Equality)) PREPO_EXP(exps.back().item);

            state.internalIndex = 2;
            PREPO_RULE(MacroCall);
          } else {
            if (!exps.back()) PREPO_NOT_OK;

            auto left = (exps.size() == 1) ? ruleNode : *exps.front().item;
            auto result = evaluateExpressions ? PrepoExpression(left == *exps.back().item) : PrepoExpression();

            exps.clear();

            if (expect(TokenType::Equality)) {
              ruleNode = std::move(result);
              PREPO_RULE(MacroCall);
            }

            PREPO_NODE(result);
          }
        } else if (rule == PrepoRuleType::MacroCall) {
          if (state.internalIndex == 0) {
            auto cache = currentState;
            auto target = expect(TokenType::Identifier);
            if (!(target && expect(TokenType::OpeningParenthesis))) {
              state.internalIndex = 2;
              currentState = cache;
              PREPO_RULE(AnyLiteral);
            }
            state.internalIndex = 1;
            ruleNode = PrepoExpression(target.raw);
            state.internalValue = new std::vector<PrepoExpression>();
            PREPO_RULE(Expression);
          } else if (state.internalIndex == 1) {
            auto target = ruleNode.string;
            auto args = ALTACORE_ANY_CAST<std::vector<PrepoExpression>*>(state.internalValue);

            if (exps.back()) {
              args->push_back(*exps.back().item);
              if (expect(TokenType::Comma)) {
                PREPO_RULE(Expression);
              }
            }

            auto localArgs = *args;
            delete args;

            if (!expect(TokenType::ClosingParenthesis)) PREPO_NOT_OK;

            if (!evaluateExpressions) PREPO_NODE(PrepoExpression());

            if (target == "defined") {
              PREPO_NODE(Prepo::defined(localArgs));
            } else {
              throw std::runtime_error("only builtin macros are currently supported");
            }
          } else {
            PREPO_EXP(exps.back().item);
          }
        } else if (rule == PrepoRuleType::Retrieval) {
          auto target = expect(TokenType::Identifier);
          if (!target) PREPO_NOT_OK;
          if (!evaluateExpressions) PREPO_NODE(PrepoExpression());
          if (definitions.find(target.raw) == definitions.end()) {
            PREPO_NODE(PrepoExpression());
          } else {
            PREPO_NODE(definitions[target.raw]);
          }
        } else if (rule == PrepoRuleType::String) {
          auto str = expect(TokenType::String);
          if (!str) PREPO_NOT_OK;
          if (!evaluateExpressions) PREPO_NODE(PrepoExpression());
          PREPO_NODE(PrepoExpression(AltaCore::Util::unescape(str.raw.substr(1, str.raw.length() - 2))));
        } else if (rule == PrepoRuleType::BooleanLiteral) {
          auto id = expect(TokenType::Identifier);
          if (!id) PREPO_NOT_OK;
          if (!evaluateExpressions) PREPO_NODE(PrepoExpression());
          if (id.raw == "true") {
            PREPO_NODE(PrepoExpression(true));
          } else if (id.raw == "false") {
            PREPO_NODE(PrepoExpression(false));
          } else {
            PREPO_NOT_OK;
          }
        } else if (rule == PrepoRuleType::And) {
          if (state.internalIndex == 0) {
            state.internalIndex = 1;
            PREPO_RULE(Equality);
          } else if (state.internalIndex == 1) {
            if (!exps.back()) PREPO_NOT_OK;

            if (!expect(TokenType::And)) PREPO_EXP(exps.back().item);

            state.internalIndex = 2;
            PREPO_RULE(Equality);
          } else {
            if (!exps.back()) PREPO_NOT_OK;

            auto left = (exps.size() == 1) ? ruleNode : *exps.front().item;
            auto result = evaluateExpressions ? PrepoExpression(left && *exps.back().item) : PrepoExpression();
            if (!result) {
              evaluateExpressions = false;
            }

            exps.clear();

            if (expect(TokenType::And)) {
              ruleNode = std::move(result);
              PREPO_RULE(Equality);
            }

            PREPO_NODE(result);
          }
        } else if (rule == PrepoRuleType::Or) {
          if (state.internalIndex == 0) {
            state.internalIndex = 1;
            PREPO_RULE(And);
          } else if (state.internalIndex == 1) {
            if (!exps.back()) PREPO_NOT_OK;

            if (!expect(TokenType::Or)) PREPO_EXP(exps.back().item);

            state.internalIndex = 2;
            PREPO_RULE(And);
          } else {
            if (!exps.back()) PREPO_NOT_OK;

            auto left = (exps.size() == 1) ? ruleNode : *exps.front().item;
            auto result = evaluateExpressions ? PrepoExpression(left || *exps.back().item) : PrepoExpression();
            if (result) {
              evaluateExpressions = false;
            }

            exps.clear();

            if (expect(TokenType::Or)) {
              ruleNode = std::move(result);
              PREPO_RULE(And);
            }

            PREPO_NODE(result);
          }
        } else if (rule == PrepoRuleType::Wrapped) {
          if (state.internalIndex == 0) {
            if (!expect(TokenType::OpeningParenthesis)) PREPO_NOT_OK;
            state.internalIndex = 1;
            PREPO_RULE(Expression);
          } else {
            if (!exps.back()) PREPO_NOT_OK;
            if (!expect(TokenType::ClosingParenthesis)) PREPO_NOT_OK;
            PREPO_EXP(exps.back().item);
          }
        } else if (rule == PrepoRuleType::AnyLiteral) {
          if (state.internalIndex == 0) {
            state.internalIndex = 1;
            PREPO_RULE_LIST(
              PrepoRuleType::BooleanLiteral,
              PrepoRuleType::Retrieval,
              PrepoRuleType::String
            );
          }

          PREPO_EXP(exps.back().item);
        }

        #undef PREPO_NOT_OK
        #undef PREPO_NODE
        #undef PREPO_RULES
        #undef PREPO_RULE_LIST
        #undef PREPO_EXP
        #undef PREPO_RULE

        next();
      }

      return root;
    };

    Parser::Parser(std::vector<Token> _tokens, ALTACORE_MAP<std::string, PrepoExpression>& _definitions, Filesystem::Path _filePath):
      tokens(_tokens),
      originalTokens(_tokens),
      definitions(_definitions),
      filePath(_filePath),
      relexer(filePath)
      {};

    void Parser::parse() {
      auto absoluteFilePath = filePath.absolutify();
      Timing::parseTimes[absoluteFilePath].start();
      std::stack<RuleStackElement> ruleStack;

      ruleStack.emplace(
        RuleType::Root,
        std::stack<RuleType>(),
        RuleState(currentState),
        std::vector<Expectation>(),
        nullptr,
        std::make_tuple(currentState, std::stack<bool>(), std::stack<bool>(), true)
      );

      auto next = [&](bool ok = false, std::vector<RuleType> rules = {}, NodeType result = nullptr) {
        auto& state = std::get<2>(ruleStack.top());
        state.iteration++;

        if (result) {
          auto& tok = tokens[state.stateAtStart.currentPosition];
          result->position.line = tok.line;
          result->position.column = tok.column;
          result->position.filePosition = tok.position;
          result->position.file = filePath;
        }

        // the null rule is never executed, it's used to jump to a different
        // state within the same rule
        if (rules.size() == 1 && rules[0] == RuleType::NullRule) return;

        auto& ruleExps = std::get<1>(ruleStack.top());
        for (auto it = rules.rbegin(); it != rules.rend(); it++) {
          ruleExps.push(*it);
        }

        if (ok && rules.size() > 0) return;

        auto oldRuleType = std::get<0>(ruleStack.top());
        auto oldState = std::get<2>(ruleStack.top());
        ruleStack.pop();

        if (ruleStack.size() < 1) return;

        auto& [newRule, newNextExps, newRuleState, newExps, ruleNode, stateCache] = ruleStack.top();

        if (!ok) {
          currentState = oldState.stateAtStart;
          if (newNextExps.size() < 1) {
            newExps.push_back(Expectation()); // push back an invalid expectation
          }
        } else if (ruleExps.size() == 0) {
          newNextExps = {};
          newExps.push_back(Expectation(oldRuleType, result));
        }
      };

      std::stack<bool> prepoLevels;
      std::stack<bool> prepoLast;
      bool advanceExp = true;

      auto saveSpecificState = [&](State state) {
        if (ruleStack.size() < 1) return;
        std::get<5>(ruleStack.top()) = std::make_tuple(state, prepoLevels, prepoLast, advanceExp);
      };
      auto saveState = [&]() {
        return saveSpecificState(currentState);
      };
      auto restoreState = [&]() {
        if (ruleStack.size() < 1) return;
        auto [state, levels, last, advance] = std::get<5>(ruleStack.top());
        currentState = state;
        prepoLevels = levels;
        prepoLast = last;
        advanceExp = advance;
      };

      auto topLevelTrue = [&]() {
        if (prepoLevels.size() < 1) return true;
        if (prepoLevels.top()) return true;
        return false;
      };

      auto foundBlock = [&]() {
        if (prepoLast.size() < 1) return true;
        if (prepoLast.top()) return true;
        return false;
      };

      while (ruleStack.size() > 0) {
        auto& [rule, nextExps, state, exps, ruleNode, stateCache] = ruleStack.top();

        if (advanceExp && nextExps.size() > 0) {
          auto nextExp = nextExps.top();
          nextExps.pop();

          ruleStack.emplace(
            nextExp,
            std::stack<RuleType>(),
            RuleState(currentState),
            std::vector<Expectation>(),
            nullptr,
            std::make_tuple(currentState, prepoLevels, prepoLast, advanceExp)
          );
          continue;
        } else if (!advanceExp) {
          advanceExp = true;
        }

        // here's the preprocessor logic,
        // it hijacks all other rules and takes
        // maximum precedence
        if (auto dir = expect(TokenType::PreprocessorDirective)) {
          auto directive = dir.raw.substr(2);
          auto currentLine = dir.line;

          auto ignoreLine = [&]() {
            while (peek().line == currentLine) {
              expectAnyToken();
            }
          };

          #define PREPO_CONTINUE { advanceExp = false; continue; }

          if (directive == "if") {
            auto expr = expectPrepoExpression();
            if (!expr) {
              ignoreLine();
            } else {
              prepoLevels.push(!!*expr);
              prepoLast.push(!!*expr);
            }
          } else if (directive == "else") {
            auto ifTok = peek();
            if (ifTok.line == currentLine && ifTok.raw == "if") {
              expectAnyToken(); // consume the "if"
              if (topLevelTrue()) {
                prepoLast.top() = false;
              } else {
                auto expr = expectPrepoExpression();
                if (!expr) {
                  ignoreLine();
                } else {
                  prepoLevels.top() = !!*expr;
                  prepoLast.top() = !!*expr;
                }
              }
            } else {
              if (topLevelTrue()) {
                prepoLast.top() = false;
              } else {
                prepoLevels.top() = true;
                prepoLast.top() = true;
              }
            }
          } else if (directive == "end") {
            auto nextTok = peek();
            if (nextTok.line == currentLine && nextTok.raw == "if") {
              expectAnyToken(); // consume the "if"
              prepoLevels.pop();
              prepoLast.pop();
            } else {
              ignoreLine();
            }
          } else if (directive == "define") {
            auto name = peek();
            if (!name || name.line != currentLine || name.type != TokenType::Identifier) {
              ignoreLine();
              PREPO_CONTINUE;
            }
            expectAnyToken(); // consume the identifier
            auto expr = expectPrepoExpression();
            definitions[name.raw] = (expr) ? *expr : PrepoExpression(nullptr);
          } else if (directive == "undefine") {
            auto name = peek();
            if (!name || name.line != currentLine || name.type != TokenType::Identifier) {
              ignoreLine();
              PREPO_CONTINUE;
            }
            expectAnyToken(); // consume the identifier
            if (definitions.find(name.raw) != definitions.end()) {
              definitions.erase(name.raw);
            }
          } else {
            ignoreLine();
          }

          PREPO_CONTINUE;

          #undef PREPO_CONTINUE
        } else if (!foundBlock()) {
          while (peek().type != TokenType::PreprocessorDirective) {
            expectAnyToken();
          }
          advanceExp = false;
          continue;
        }

        /*
         * note about early returns in front-recursive rules:
         *
         * all front-recursive rules expect another expression as
         * their first expectation and thus lead to semi-recursion. they
         * also expect a differentiating token like `+` or `(` or `if` etc.
         * for those rules, in order to optimize parse times (literally in half,
         * probably even exponentially), if their left-hand expression is found
         * but their differentiating token is not, they simply return their
         * left-hand expression and pretend to succeed. this, however, is fine
         * because the parser doesn't really care what rules succeed or fail,
         * only the results of those rules. so if, for example, while trying to
         * find an AdditionOrSubtraction expression we find a FunctionCallOrSubscript and
         * then we don't find `+` or `-`, the AdditionOrSubtraction expression will
         * simply return the FunctionCallOrSubscript result as if it were its own. the
         * parser won't care that AdditionOrSubtraction is returning
         * an AST::FunctionCallExpression instead of a AST::BinaryOperation, only
         * that it did return a result, and it'll pass the result back to whatever
         * rule invoked AdditionOrSubtraction.
         */

        if (rule == RuleType::Root) {
          std::vector<RuleType> stmtType = {
            RuleType::ModuleOnlyStatement,
            RuleType::Statement,
          };

          // logic for the initial call
          if (state.iteration == 0) ACP_RULES(std::move(stmtType));

          // basically a while loop that continues as long statements are available
          if (exps.back()) ACP_RULES(std::move(stmtType));

          exps.pop_back(); // remove the last (implicitly invalid) expectation

          std::vector<std::shared_ptr<AST::StatementNode>> statements;
          for (auto& exp: exps) {
            auto stmt = std::dynamic_pointer_cast<AST::StatementNode>(*exp.item);
            if (stmt == nullptr) throw std::runtime_error("AST node given was not of the expected type");
            statements.push_back(stmt);
          }

          if (currentState.currentPosition < tokens.size()) {
            auto& tok = tokens[farthestRule.currentState.currentPosition];
            throw Errors::ParsingError("input not completely parsed; assuming failure", Errors::Position(tok.line, tok.column, filePath));
          }

          root = std::make_shared<AST::RootNode>(statements);

          next(true);
          break;
        } else if (rule == RuleType::Statement) {
          if (state.iteration == 0) ACP_RULE_LIST(
            RuleType::FunctionDefinition,
            RuleType::FunctionDeclaration,
            RuleType::ReturnDirective,
            RuleType::ConditionalStatement,
            RuleType::Block,
            RuleType::ClassDefinition,
            RuleType::Structure,
            RuleType::WhileLoop,
            RuleType::ForLoop,
            RuleType::RangedFor,
            RuleType::TypeAlias,
            RuleType::Expression,

            // general attributes must come last because
            // they're supposed to be able to interpreted as part of
            // other statements that accept attributes if any such
            // statement is present
            RuleType::GeneralAttribute
          );

          if (!exps.back()) ACP_NOT_OK;

          while (expect(TokenType::Semicolon)); // optional

          auto& exp = exps.back();
          auto ret = std::dynamic_pointer_cast<AST::StatementNode>(*exp.item);
          if (exp.type == RuleType::Expression) {
            auto expr = std::dynamic_pointer_cast<AST::ExpressionNode>(*exp.item);
            if (expr == nullptr) throw std::runtime_error("wtf");
            ret = std::make_shared<AST::ExpressionStatement>(expr);
          }

          ACP_NODE(ret);
        } else if (rule == RuleType::Expression) {
          if (state.internalIndex == 0) {
            state.internalIndex = 1;
            ACP_RULE(VariableDefinition);
          } else {
            if (!exps.back()) ACP_NOT_OK;
            auto expr = *exps.back().item;
            ACP_NODE(expr);
          }
        } else if (rule == RuleType::FunctionDefinition) {
          if (state.internalIndex == 0) {
            state.internalIndex = 1;
            ACP_RULE(Attribute);
          } else if (state.internalIndex == 1) {
            if (exps.back()) ACP_RULE(Attribute);

            exps.pop_back();

            auto funcDef = std::make_shared<AST::FunctionDefinitionNode>();
            funcDef->modifiers = expectModifiers(ModifierTargetType::Function);

            ruleNode = std::move(funcDef);
            state.internalIndex = 2;
            ACP_RULE(Attribute);
          } else if (state.internalIndex == 2) {
            if (exps.back()) ACP_RULE(Attribute);

            exps.pop_back();

            auto funcDef = std::dynamic_pointer_cast<AST::FunctionDefinitionNode>(ruleNode);

            for (auto& exp: exps) {
              funcDef->attributes.push_back(std::dynamic_pointer_cast<AST::AttributeNode>(*exp.item));
            }
            exps.clear();

            if (!expectKeyword("function")) ACP_NOT_OK;

            auto name = expect(TokenType::Identifier);
            if (!name) ACP_NOT_OK;
            funcDef->name = name.raw;

            if (!expect(TokenType::OpeningParenthesis)) ACP_NOT_OK;

            state.internalIndex = 3;
            ACP_RULE(Parameter);
          } else if (state.internalIndex == 3) {
            auto funcDef = std::dynamic_pointer_cast<AST::FunctionDefinitionNode>(ruleNode);

            if (exps.back()) {
              std::shared_ptr<AST::Parameter> parameter = std::dynamic_pointer_cast<AST::Parameter>(*exps.back().item);
              if (parameter == nullptr) throw std::runtime_error("oh no.");
              funcDef->parameters.push_back(parameter);

              exps.pop_back();

              if (expect(TokenType::Comma)) ACP_RULE(Parameter);
            }

            exps.clear(); // we don't need those parameter expecatations anymore

            if (!expect(TokenType::ClosingParenthesis)) ACP_NOT_OK;
            if (!expect(TokenType::Colon)) ACP_NOT_OK;

            state.internalIndex = 4;
            ACP_RULE(Type);
          } else if (state.internalIndex == 4) {
            auto funcDef = std::dynamic_pointer_cast<AST::FunctionDefinitionNode>(ruleNode);

            if (!exps.back()) ACP_NOT_OK;
            funcDef->returnType = std::dynamic_pointer_cast<AST::Type>(*exps.back().item);

            state.internalIndex = 5;
            ACP_RULE(Block);
          } else {
            auto funcDef = std::dynamic_pointer_cast<AST::FunctionDefinitionNode>(ruleNode);

            if (!exps.back()) ACP_NOT_OK;
            funcDef->body = std::dynamic_pointer_cast<AST::BlockNode>(*exps.back().item);

            ACP_NODE(std::move(funcDef));
          }
        } else if (rule == RuleType::Parameter) {
          if (state.internalIndex == 0) {
            state.internalIndex = 1;
            ACP_RULE(Attribute);
          } else if (state.internalIndex == 1) {
            if (exps.back()) ACP_RULE(Attribute);

            exps.pop_back(); // remove the last (implicitly invalid) expectation

            auto param = std::make_shared<AST::Parameter>();

            for (auto& exp: exps) {
              param->attributes.push_back(std::dynamic_pointer_cast<AST::AttributeNode>(*exp.item));
            }

            auto name = expect(TokenType::Identifier);
            if (!name) ACP_NOT_OK;
            param->name = name.raw;

            if (!expect(TokenType::Colon)) ACP_NOT_OK;

            ruleNode = std::move(param);
            state.internalIndex = 2;

            typesToIgnore.insert("any");
            ACP_RULE(Type);
          } else {
            typesToIgnore.erase("any");

            bool isAny = false;
            if (!exps.back()) {
              if (expectKeyword("any")) {
                isAny = true;
              } else {
                ACP_NOT_OK;
              }
            }

            auto param = std::dynamic_pointer_cast<AST::Parameter>(ruleNode);

            if (isAny) {
              param->type = std::make_shared<AST::Type>();
              param->type->isAny = true;
            } else {
              param->type = std::dynamic_pointer_cast<AST::Type>(*exps.back().item);
            }

            auto savedState = currentState;
            if (expect(TokenType::Dot) && expect(TokenType::Dot) && expect(TokenType::Dot)) {
              param->isVariable = true;
            } else {
              currentState = savedState;
            }

            ACP_NODE(std::move(param));
          }
        } else if (rule == RuleType::StrictAccessor) {
          if (state.internalIndex == 0) {
            state.internalIndex = 1;
            ACP_RULE(Fetch);
          } else if (state.internalIndex == 2) {
            auto acc = std::dynamic_pointer_cast<AST::Accessor>(ruleNode);

            if (!exps.back()) {
              if (acc->genericArguments.size() < 1) {
                restoreState();
                acc->genericArguments.clear();
                ACP_NODE(acc);
              }
            } else {
              acc->genericArguments.push_back(std::dynamic_pointer_cast<AST::Type>(*exps.back().item));

              if (expect(TokenType::Comma)) {
                ACP_RULE(Type);
              }
            }

            if (!expect(TokenType::ClosingAngleBracket)) {
              restoreState();
              acc->genericArguments.clear();
            }

            ACP_NODE(acc);
          } else {
            if (!exps.back()) ACP_NOT_OK;

            auto savedState = currentState;
            if (!expect(TokenType::Dot)) ACP_EXP(exps.back().item);

            auto query = expect(TokenType::Identifier);
            if (!query) {
              currentState = savedState;
              ACP_EXP(exps.back().item);
            }

            auto acc = std::make_shared<AST::Accessor>(std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item), query.raw);

            savedState = currentState;
            while (expect(TokenType::Dot)) {
              query = expect(TokenType::Identifier);
              if (!query) {
                currentState = savedState;
                break;
              }
              acc = std::make_shared<AST::Accessor>(acc, query.raw);
              savedState = currentState;
            }

            saveState();

            if (expect(TokenType::OpeningAngleBracket)) {
              ruleNode = std::move(acc);
              state.internalIndex = 2;
              ACP_RULE(Type);
            }

            ACP_NODE(acc);
          }
        } else if (rule == RuleType::Type) {
          if (state.internalIndex == 0) {
            auto type = std::make_shared<AST::Type>();

            auto modifiers = expectModifiers(ModifierTargetType::Type);
            type->modifiers.push_back(0);
            for (auto& modifier: modifiers) {
              auto& bitFlag = type->modifiers.back();
              if (modifier == "ptr") {
                bitFlag |= (uint8_t)AST::TypeModifierFlag::Pointer;
                type->modifiers.push_back(0);
              } else if (modifier == "ref") {
                bitFlag |= (uint8_t)AST::TypeModifierFlag::Reference;
                type->modifiers.push_back(0);
              } else {
                uint8_t flag = 0;
                if (modifier == "const") {
                  flag = (uint8_t)AST::TypeModifierFlag::Constant;
                } else if (modifier == "signed") {
                  flag = (uint8_t)AST::TypeModifierFlag::Signed;
                } else if (modifier == "unsigned") {
                  flag = (uint8_t)AST::TypeModifierFlag::Unsigned;
                } else if (modifier == "long") {
                  flag = (uint8_t)AST::TypeModifierFlag::Long;
                } else if (modifier == "short") {
                  flag = (uint8_t)AST::TypeModifierFlag::Short;
                }
                if (bitFlag & flag) {
                  type->modifiers.push_back(0);
                }
                type->modifiers.back() |= flag;
              }
            }
            if (type->modifiers.back() == 0) {
              type->modifiers.pop_back();
            }

            ruleNode = std::move(type);

            if (!expect(TokenType::OpeningParenthesis)) {
              state.internalIndex = 3;
              ACP_RULE(StrictAccessor);
            }

            state.internalIndex = 1;

            ACP_RULE(Type);
          } else if (state.internalIndex == 1) {
            auto type = std::dynamic_pointer_cast<AST::Type>(ruleNode);

            if (exps.back()) {
              type->parameters.push_back({
                std::dynamic_pointer_cast<AST::Type>(*exps.back().item),
                false,
                "",
              });

              if (expect(TokenType::Comma)) {
                exps.pop_back();

                ACP_RULE(Type);
              }
            }

            exps.pop_back();

            if (!expect(TokenType::ClosingParenthesis)) ACP_NOT_OK;

            if (expect(TokenType::Returns)) {
              // if we continue, we're parsing a function pointer type
              type->isFunction = true;

              state.internalIndex = 2;
              ACP_RULE(Type);
            } else if (type->parameters.size() > 1) {
              // somehow, we detected parameters, but there's no ACP_NODE((indicator,
              // so this isn't a type
              ACP_NOT_OK;
            } else {
              auto otherType = std::get<0>(type->parameters[0]);
              otherType->modifiers.insert(otherType->modifiers.begin(), type->modifiers.begin(), type->modifiers.end());
              ACP_NODE(otherType);
            }
          } else if (state.internalIndex == 2) {
            if (!exps.back()) ACP_NOT_OK;

            auto type = std::dynamic_pointer_cast<AST::Type>(ruleNode);

            type->returnType = std::dynamic_pointer_cast<AST::Type>(*exps.back().item);
            ACP_NODE(type);
          } else {
            auto type = std::dynamic_pointer_cast<AST::Type>(ruleNode);

            if (!exps.back()) ACP_NOT_OK;

            auto expr = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);

            if (expr->nodeType() == AST::NodeType::Fetch) {
              auto id = std::dynamic_pointer_cast<AST::Fetch>(expr);
              auto name = id->query;
              if (typesToIgnore.find(name) != typesToIgnore.end()) ACP_NOT_OK;
              if (
                name == "int" ||
                name == "byte" ||
                name == "char" ||
                name == "bool" ||
                name == "void" ||
                name == "float" ||
                name == "double"
              ) {
                type->name = name;
                type->isNative = true;
              } else {
                type->lookup = expr;
                type->isNative = false;
              }
            } else {
              type->lookup = expr;
              type->isNative = false;
            }

            ACP_NODE(type);
          }
        } else if (rule == RuleType::IntegralLiteral) {
          auto integer = expect(TokenType::Integer);
          if (!integer) ACP_NOT_OK;
          ACP_NODE(std::make_shared<AST::IntegerLiteralNode>(integer.raw));
        } else if (rule == RuleType::ReturnDirective) {
          if (state.internalIndex == 0) {
            if (!expectKeyword("return")) ACP_NOT_OK;
            state.internalIndex = 1;
            ACP_RULE(Expression);
          } else {
            std::shared_ptr<AST::ExpressionNode> expr = nullptr;
            if (exps.back()) {
              expr = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);
            }
            ACP_NODE(std::make_shared<AST::ReturnDirectiveNode>(expr));
          }
        } else if (rule == RuleType::Block) {
          if (state.internalIndex == 0) {
            if (!expect(TokenType::OpeningBrace)) ACP_NOT_OK;

            state.internalIndex = 1;
            ruleNode = std::make_shared<AST::BlockNode>();

            ACP_RULE(Statement);
          } else {
            auto block = std::dynamic_pointer_cast<AST::BlockNode>(ruleNode);

            if (exps.back()) {
              block->statements.push_back(std::dynamic_pointer_cast<AST::StatementNode>(*exps.back().item));

              exps.pop_back();
              ACP_RULE(Statement);
            }

            if (!expect(TokenType::ClosingBrace)) ACP_NOT_OK;
            ACP_NODE(block);
          }
        } else if (rule == RuleType::VariableDefinition) {
          if (state.internalIndex == 0) {
            auto varDef = std::make_shared<AST::VariableDefinitionExpression>();

            const auto saved = currentState;
            varDef->modifiers = expectModifiers(ModifierTargetType::Variable);

            if (!expectKeyword("let") && !expectKeyword("var")) {
              currentState = saved;
              state.internalIndex = 3;
              ACP_RULE(Assignment);
            }

            auto name = expect(TokenType::Identifier);
            if (!name) ACP_NOT_OK;
            varDef->name = name.raw;

            ruleNode = std::move(varDef);

            if (expect(TokenType::EqualSign)) {
              state.internalIndex = 2;
              ACP_RULE(Expression);
            } else if (!expect(TokenType::Colon)) {
              ACP_NOT_OK;
            }

            state.internalIndex = 1;
            ACP_RULE(Type);
          } else if (state.internalIndex == 1) {
            if (!exps.back()) ACP_NOT_OK;
            auto varDef = std::dynamic_pointer_cast<AST::VariableDefinitionExpression>(ruleNode);
            varDef->type = std::dynamic_pointer_cast<AST::Type>(*exps.back().item);

            if (expect(TokenType::EqualSign)) {
              state.internalIndex = 2;
              ACP_RULE(Expression);
            } else {
              ACP_NODE(varDef);
            }
          } else if (state.internalIndex == 3) {
            ACP_EXP(exps.back().item);
          } else {
            // we're expecting a value to initialize the variable
            if (!exps.back()) ACP_NOT_OK;
            auto varDef = std::dynamic_pointer_cast<AST::VariableDefinitionExpression>(ruleNode);
            varDef->initializationExpression = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);

            ACP_NODE(varDef);
          }
        } else if (rule == RuleType::Fetch) {
          if (state.internalIndex == 0) {
            auto id = expect(TokenType::Identifier);
            if (!id) ACP_NOT_OK;

            auto fetch = std::make_shared<AST::Fetch>(id.raw);

            saveState();
            if (expect(TokenType::OpeningAngleBracket)) {
              state.internalIndex = 1;
              ruleNode = std::move(fetch);
              ACP_RULE(Type);
            }

            ACP_NODE(fetch);
          } else {
            auto fetch = std::dynamic_pointer_cast<AST::Fetch>(ruleNode);

            if (!exps.back()) {
              if (fetch->genericArguments.size() < 1) {
                restoreState();
                fetch->genericArguments.clear();
                ACP_NODE(fetch);
              }
            } else {
              fetch->genericArguments.push_back(std::dynamic_pointer_cast<AST::Type>(*exps.back().item));

              if (expect(TokenType::Comma)) {
                ACP_RULE(Type);
              }
            }

            if (!expect(TokenType::ClosingAngleBracket)) {
              restoreState();
              fetch->genericArguments.clear();
            }

            ACP_NODE(fetch);
          }
        } else if (rule == RuleType::Assignment) {
          if (state.internalIndex == 0) {
            state.internalIndex = 1;
            ACP_RULE(VerbalConditionalExpression);
          } else if (state.internalIndex == 1) {
            if (!exps.back()) ACP_NOT_OK;

            using AT = AST::AssignmentType;

            AT type = AT::Simple;

            if (expect(TokenType::EqualSign)) {
              type = AT::Simple;
            } else if (expect(TokenType::PlusEquals)) {
              type = AT::Addition;
            } else if (expect(TokenType::MinusEquals)) {
              type = AT::Subtraction;
            } else if (expect(TokenType::TimesEquals)) {
              type = AT::Multiplication;
            } else if (expect(TokenType::DividedEquals)) {
              type = AT::Division;
            } else if (expect(TokenType::ModuloEquals)) {
              type = AT::Modulo;
            } else if (expect(TokenType::LeftShiftEquals)) {
              type = AT::LeftShift;
            } else if (expect(TokenType::RightShiftEquals)) {
              type = AT::RightShift;
            } else if (expect(TokenType::BitwiseAndEquals)) {
              type = AT::BitwiseAnd;
            } else if (expect(TokenType::BitwiseOrEquals)) {
              type = AT::BitwiseOr;
            } else if (expect(TokenType::BitwiseAndEquals)) {
              type = AT::BitwiseXor;
            }else {
              ACP_EXP(exps.back().item);
            }

            ruleNode = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);
            state.internalIndex = 2;
            state.internalValue = type;

            ACP_RULE(Assignment);
          } else {
            if (!exps.back()) ACP_NOT_OK;

            auto lhs = std::dynamic_pointer_cast<AST::ExpressionNode>(ruleNode);
            auto node = std::make_shared<AST::AssignmentExpression>(lhs, std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item));
            node->type = ALTACORE_ANY_CAST<AST::AssignmentType>(state.internalValue);

            ACP_NODE(node);
          }
        } else if (rule == RuleType::AdditionOrSubtraction) {
          if (expectBinaryOperation(rule, RuleType::MultiplicationOrDivisionOrModulo, {
            TokenType::PlusSign,
            TokenType::MinusSign,
          }, {
            AST::OperatorType::Addition,
            AST::OperatorType::Subtraction,
          }, state, exps, ruleNode, next, saveState, restoreState)) {
            continue;
          }
        } else if (rule == RuleType::MultiplicationOrDivisionOrModulo) {
          if (expectBinaryOperation(rule, RuleType::Cast, {
            TokenType::Asterisk,
            TokenType::ForwardSlash,
            TokenType::Percent,
          }, {
            AST::OperatorType::Multiplication,
            AST::OperatorType::Division,
            AST::OperatorType::Modulo,
          }, state, exps, ruleNode, next, saveState, restoreState)) {
            continue;
          }
        } else if (rule == RuleType::ModuleOnlyStatement) {
          if (state.iteration == 0) {
            ACP_RULE_LIST(
              RuleType::Import,
              RuleType::Export
            );
          } else {
            while (expect(TokenType::Semicolon)); // optional
            if (!exps.back()) ACP_NOT_OK;
            ACP_EXP(exps.back().item);
          }
        } else if (rule == RuleType::Import) {
          if (!expectKeyword("import")) ACP_NOT_OK;
          bool isAlias = false;
          std::string modName;
          std::vector<std::pair<std::string, std::string>> imports;
          std::string alias;
          if (expect(TokenType::OpeningBrace)) {
            auto importExp = expect(TokenType::Identifier);
            while (importExp) {
              std::string aliasString = "";
              if (expectKeyword("as")) {
                auto aliasExp = expect(TokenType::Identifier);
                if (!aliasExp) break;
                aliasString = aliasExp.raw;
              }
              imports.push_back({ importExp.raw, aliasString });
              if (!expect(TokenType::Comma)) break;
              importExp = expect(TokenType::Identifier);
            }
            expect(TokenType::Comma); // optional trailing comma
            if (!expect(TokenType::ClosingBrace)) ACP_NOT_OK;
            if (!expectKeyword("from")) ACP_NOT_OK;
            auto mod = expect(TokenType::String);
            if (!mod) ACP_NOT_OK;
            modName = mod.raw.substr(1, mod.raw.length() - 2);
          } else {
            if (auto mod = expect(TokenType::String)) {
              isAlias = true;
              modName = mod.raw.substr(1, mod.raw.length() - 2);
              if (!expectKeyword("as")) ACP_NOT_OK;
              auto aliasExp = expect(TokenType::Identifier);
              if (!aliasExp) ACP_NOT_OK;
              alias = aliasExp.raw;
            } else {
              auto importExp = expect(TokenType::Identifier);
              bool from = false;
              while (importExp) {
                if (importExp.raw == "from") {
                  from = true;
                  break;
                }
                std::string aliasString = "";
                if (expectKeyword("as")) {
                  auto aliasExp = expect(TokenType::Identifier);
                  if (!aliasExp) break;
                  aliasString = aliasExp.raw;
                }
                imports.push_back({ importExp.raw, aliasString });
                if (!expect(TokenType::Comma)) break;
                importExp = expect(TokenType::Identifier);
              }
              if (imports.size() == 0) ACP_NOT_OK; // braced cherry-pick imports can have 0, but not freestyle cherry-pick imports
              expect(TokenType::Comma); // optional trailing comma
              if (!from) {
                // we probably already got it in the while loop, but just in case, check for it here
                if (!expectKeyword("from")) ACP_NOT_OK;
              }
              auto module = expect(TokenType::String);
              if (!module) ACP_NOT_OK;
              modName = module.raw.substr(1, module.raw.length() - 2);
            }
          }
          modName = Util::unescape(modName);
          std::shared_ptr<AST::ImportStatement> node = nullptr;
          if (isAlias) {
            node = std::make_shared<AST::ImportStatement>(modName, alias);
          } else {
            node = std::make_shared<AST::ImportStatement>(modName, imports);
          }
          Timing::parseTimes[absoluteFilePath].stop();
          node->parse(filePath);
          Timing::parseTimes[absoluteFilePath].start();
          ACP_NODE(node);
        } else if (rule == RuleType::BooleanLiteral) {
          if (expectKeyword("true")) {
            ACP_NODE((std::make_shared<AST::BooleanLiteralNode>(true)));
          } else if (expectKeyword("false")) {
            ACP_NODE((std::make_shared<AST::BooleanLiteralNode>(false)));
          }
        } else if (rule == RuleType::FunctionCallOrSubscriptOrAccessorOrPostIncDec) {
          if (state.internalIndex == 0) {
            state.internalIndex = 1;
            ACP_RULE(ClassInstantiation);
          } else if (state.internalIndex == 1) {
            if (!exps.back() && !ruleNode) ACP_NOT_OK;

            auto target = std::dynamic_pointer_cast<AST::ExpressionNode>(ruleNode ? ruleNode : *exps.back().item);

            bool isCall = false;
            bool isSubscript = false;
            bool isAccessor = false;
            bool isPostIncrement = false;
            bool isPostDecrement = false;

            saveState();

            if (expect(TokenType::OpeningParenthesis)) {
              isCall = true;
            } else if (expect(TokenType::OpeningSquareBracket)) {
              isSubscript = true;
            } else if (expect(TokenType::Dot) && expect(TokenType::Identifier)) {
              isAccessor = true;
            } else if (expect(TokenType::Increment)) {
              isPostIncrement = true;
            } else if (expect(TokenType::Decrement)) {
              isPostDecrement = true;
            } else {
              restoreState();
              ACP_NODE(target);
            }

            if (isCall) {
              auto funcCall = std::make_shared<AST::FunctionCallExpression>();
              funcCall->target = target;

              auto tmpState = currentState;
              auto name = expect(TokenType::Identifier);
              if (name && !expect(TokenType::Colon)) {
                name = Token(); // constructs an invalid expectation
                currentState = tmpState;
              }
              funcCall->arguments.push_back({
                (name) ? name.raw : "",
                nullptr,
              });

              ruleNode = std::move(funcCall);
              state.internalIndex = 2;

              ACP_RULE(Expression);
            } else if (isSubscript) {
              auto subs = std::make_shared<AST::SubscriptExpression>();
              subs->target = target;

              ruleNode = std::move(subs);
              state.internalIndex = 3;

              ACP_RULE(Expression);
            } else if (isAccessor) {
              ruleNode = target;
              state.internalIndex = 5;

              restoreState(); // state 5 starts expecting dots and identifiers

              ACP_RULE(NullRule);
            } else {
              auto op = std::make_shared<AST::UnaryOperation>();
              op->target = target;

              if (isPostIncrement) {
                op->type = AST::UOperatorType::PostIncrement;
              } else {
                op->type = AST::UOperatorType::PostDecrement;
              }

              ruleNode = std::move(op);

              ACP_RULE(NullRule);
            }
          } else if (state.internalIndex == 2) {
            auto callState = std::dynamic_pointer_cast<AST::FunctionCallExpression>(ruleNode);

            if (exps.back()) {
              callState->arguments.back().second = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);
              if (expect(TokenType::Comma)) {
                auto tmpState = currentState;
                auto name = expect(TokenType::Identifier);
                if (name && !expect(TokenType::Colon)) {
                  name = Token(); // constructs an invalid expectation
                  currentState = tmpState;
                }

                callState->arguments.push_back({
                  (name) ? name.raw : "",
                  nullptr,
                });

                ACP_RULE(Expression);
              }
            } else {
              callState->arguments.pop_back();
            }

            if (!expect(TokenType::ClosingParenthesis)) ACP_NOT_OK;

            state.internalIndex = 1;
            ACP_RULE(NullRule);
          } else if (state.internalIndex == 3) {
            auto subs = std::dynamic_pointer_cast<AST::SubscriptExpression>(ruleNode);

            if (!exps.back()) ACP_NOT_OK; // TODO: error recovery

            if (!expect(TokenType::ClosingSquareBracket)) ACP_NOT_OK;

            subs->index = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);

            state.internalIndex = 1;
            ACP_RULE(NullRule);
          } else if (state.internalIndex == 4) {
            auto acc = std::dynamic_pointer_cast<AST::Accessor>(ruleNode);

            if (!acc) {
              restoreState();
              ACP_NODE(ruleNode);
            }

            if (!exps.back()) {
              if (acc->genericArguments.size() < 1) {
                restoreState();
                acc->genericArguments.clear();
                ACP_NODE(acc);
              }
            } else {
              acc->genericArguments.push_back(std::dynamic_pointer_cast<AST::Type>(*exps.back().item));

              if (expect(TokenType::Comma)) {
                ACP_RULE(Type);
              }
            }

            if (!expect(TokenType::ClosingAngleBracket)) {
              restoreState();
              acc->genericArguments.clear();
            }

            state.internalIndex = 1;
            ACP_RULE(NullRule); // use a null rule to go back to the first branch
          } else if (state.internalIndex == 5) {
            auto target = std::dynamic_pointer_cast<AST::ExpressionNode>(ruleNode);

            Token id;
            saveState();
            while (expect(TokenType::Dot) && (id = expect(TokenType::Identifier))) {
              ruleNode = std::make_shared<AST::Accessor>(target, id.raw);
              target = std::dynamic_pointer_cast<AST::ExpressionNode>(ruleNode);
              saveState();
            }

            restoreState();

            saveState();

            if (expect(TokenType::OpeningAngleBracket)) {
              state.internalIndex = 4;
              ACP_RULE(Type);
            }

            state.internalIndex = 1;
            ACP_RULE(NullRule);
          }
        } else if (rule == RuleType::String) {
          auto raw = expect(TokenType::String);
          if (!raw) ACP_NOT_OK;
          ACP_NODE((std::make_shared<AST::StringLiteralNode>(Util::unescape(raw.raw.substr(1, raw.raw.length() - 2)))));
        } else if (rule == RuleType::Character) {
          auto raw = expect(TokenType::Character);
          if (!raw) ACP_NOT_OK;
          auto cont = raw.raw.substr(1, raw.raw.length() - 2);
          ACP_NODE((std::make_shared<AST::CharacterLiteralNode>((cont.length() == 2) ? cont[1] : cont[0], cont.length() == 2)));
        } else if (rule == RuleType::FunctionDeclaration) {
          if (state.internalIndex == 0) {
            if (!expectKeyword("declare")) ACP_NOT_OK;

            auto funcDecl = std::make_shared<AST::FunctionDeclarationNode>();
            funcDecl->modifiers = expectModifiers(ModifierTargetType::Function);

            if (!expectKeyword("function")) ACP_NOT_OK;

            auto name = expect(TokenType::Identifier);
            if (!name) ACP_NOT_OK;
            funcDecl->name = name.raw;

            if (!expect(TokenType::OpeningParenthesis)) ACP_NOT_OK;

            ruleNode = std::move(funcDecl);
            state.internalIndex = 1;
            ACP_RULE(Parameter);
          } else if (state.internalIndex == 1) {
            auto funcDecl = std::dynamic_pointer_cast<AST::FunctionDeclarationNode>(ruleNode);

            if (exps.back()) {
              std::shared_ptr<AST::Parameter> parameter = std::dynamic_pointer_cast<AST::Parameter>(*exps.back().item);
              if (parameter == nullptr) throw std::runtime_error("oh no.");
              funcDecl->parameters.push_back(parameter);

              exps.pop_back();

              if (expect(TokenType::Comma)) {
                ACP_RULE(Parameter);
              }
            }

            exps.clear(); // we don't need those parameter expecatations anymore

            if (!expect(TokenType::ClosingParenthesis)) ACP_NOT_OK;
            if (!expect(TokenType::Colon)) ACP_NOT_OK;

            state.internalIndex = 2;
            ACP_RULE(Type);
          } else {
            auto funcDecl = std::dynamic_pointer_cast<AST::FunctionDeclarationNode>(ruleNode);

            if (!exps.back()) ACP_NOT_OK;
            funcDecl->returnType = std::dynamic_pointer_cast<AST::Type>(*exps.back().item);

            ACP_NODE((std::move(funcDecl)));
          }
        } else if (rule == RuleType::Attribute) {
          if (state.internalIndex == 0) {
            if (!expect(TokenType::AtSign)) ACP_NOT_OK;

            auto attr = std::make_shared<AST::AttributeNode>();

            auto idExp = expect(TokenType::Identifier);
            while (idExp) {
              attr->accessors.push_back(idExp.raw);
              if (!expect(TokenType::Dot)) break;
              idExp = expect(TokenType::Identifier);
            }
            if (attr->accessors.size() == 0) ACP_NOT_OK;

            if (expect(TokenType::OpeningParenthesis)) {
              state.internalIndex = 1;
              ruleNode = std::move(attr);

              ACP_RULE(AnyLiteral);
            } else {
              ACP_NODE((std::move(attr)));
            }
          } else {
            auto attr = std::dynamic_pointer_cast<AST::AttributeNode>(ruleNode);

            if (exps.back()) {
              attr->arguments.push_back(std::dynamic_pointer_cast<AST::LiteralNode>(*exps.back().item));

              if (expect(TokenType::Comma)) {
                ACP_RULE(AnyLiteral);
              }
            }

            if (!expect(TokenType::ClosingParenthesis)) ACP_NOT_OK;

            ACP_NODE((attr));
          }
        } else if (rule == RuleType::GeneralAttribute) {
          if (state.internalIndex == 0) {
            if (!expect(TokenType::AtSign)) ACP_NOT_OK;
            state.internalIndex = 1;
            ACP_RULE(Attribute);
          } else {
            if (!exps.back()) ACP_NOT_OK;
            ACP_NODE((std::make_shared<AST::AttributeStatement>(std::dynamic_pointer_cast<AST::AttributeNode>(*exps.back().item))));
          }
        } else if (rule == RuleType::AnyLiteral) {
          if (state.internalIndex == 0) {
            state.internalIndex = 1;
            ACP_RULE_LIST(
              RuleType::Sizeof,
              RuleType::IntegralLiteral,
              RuleType::BooleanLiteral,
              RuleType::String,
              RuleType::Character,
              RuleType::DecimalLiteral
            );
          } else {
            ACP_EXP(exps.back().item);
          }
        } else if (rule == RuleType::ConditionalStatement) {
          if (state.internalIndex == 0) {
            if (!expectKeyword("if")) ACP_NOT_OK;

            state.internalIndex = 1;

            ACP_RULE(Expression);
          } else if (state.internalIndex == 1) {
            if (!exps.back()) ACP_NOT_OK;

            auto cond = std::make_shared<AST::ConditionalStatement>();
            cond->primaryTest = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);

            ruleNode = std::move(cond);
            saveState();
            state.internalIndex = 2;

            ACP_RULE(Statement);
          } else if (state.internalIndex == 2) {
            if (!exps.back()) ACP_NOT_OK;

            auto intern = std::dynamic_pointer_cast<AST::ConditionalStatement>(ruleNode);

            intern->primaryResult = std::dynamic_pointer_cast<AST::StatementNode>(*exps.back().item);

            saveState();
            if (expectKeyword("else")) {
              if (expectKeyword("if")) {
                state.internalIndex = 3;
                ACP_RULE(Expression);
              } else {
                state.internalIndex = 5;
                ACP_RULE(Statement);
              }
            }

            ACP_NODE(intern);
          } else if (state.internalIndex == 3) {
            auto intern = std::dynamic_pointer_cast<AST::ConditionalStatement>(ruleNode);

            if (!exps.back()) {
              restoreState();
              ACP_NODE(intern);
            }

            intern->alternatives.push_back({
              std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item),
              nullptr,
            });

            state.internalIndex = 4;

            ACP_RULE(Statement);
          } else if (state.internalIndex == 4) {
            auto intern = std::dynamic_pointer_cast<AST::ConditionalStatement>(ruleNode);

            if (!exps.back()) {
              restoreState();
              intern->alternatives.pop_back();
              ACP_NODE(intern);
            }

            intern->alternatives.back().second = std::dynamic_pointer_cast<AST::StatementNode>(*exps.back().item);

            saveState();
            if (expectKeyword("else")) {
              if (expectKeyword("if")) {
                state.internalIndex = 3;
                ACP_RULE(Expression);
              } else {
                state.internalIndex = 5;
                ACP_RULE(Statement);
              }
            }

            ACP_NODE(intern);
          } else if (state.internalIndex == 5) {
            auto intern = std::dynamic_pointer_cast<AST::ConditionalStatement>(ruleNode);

            if (!exps.back()) {
              restoreState();
              ACP_NODE(intern);
            }

            intern->finalResult = std::dynamic_pointer_cast<AST::StatementNode>(*exps.back().item);

            ACP_NODE(intern);
          }
        } else if (rule == RuleType::VerbalConditionalExpression) {
          if (state.internalIndex == 0) {
            state.internalIndex = 1;
            ACP_RULE(PunctualConditonalExpression);
          } else if (state.internalIndex == 1) {
            if (!exps.back()) ACP_NOT_OK;

            auto stateCache = currentState;
            if (!expectKeyword("if")) ACP_EXP(exps.back().item);

            auto cond = std::make_shared<AST::ConditionalExpression>();
            cond->primaryResult = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);

            ruleNode = std::move(cond);
            saveSpecificState(stateCache);
            state.internalIndex = 2;

            ACP_RULE(Expression);
          } else if (state.internalIndex == 2) {
            auto ruleState = std::dynamic_pointer_cast<AST::ConditionalExpression>(ruleNode);

            if (!exps.back()) {
              restoreState();
              ACP_NODE(ruleState->primaryResult);
            }

            if (!expectKeyword("else")) {
              restoreState();
              ACP_NODE(ruleState->primaryResult);
            }

            ruleState->test = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);

            state.internalIndex = 3;
            ACP_RULE(PunctualConditonalExpression);
          } else if (state.internalIndex == 3) {
            auto ruleState = std::dynamic_pointer_cast<AST::ConditionalExpression>(ruleNode);

            if (!exps.back()) {
              restoreState();
              ACP_NODE(ruleState->primaryResult);
            }

            ruleState->secondaryResult = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);

            auto stateCache = currentState;
            if (expectKeyword("if")) {
              auto newCond = std::make_shared<AST::ConditionalExpression>();
              newCond->primaryResult = ruleState;

              state.internalValue = true;
              ruleNode = newCond;
              saveSpecificState(stateCache);
              state.internalIndex = 2;

              ACP_RULE(Expression);
            }

            ACP_NODE(ruleState);
          }
        } else if (rule == RuleType::PunctualConditonalExpression) {
          if (state.internalIndex == 0) {
            state.internalIndex = 1;
            ACP_RULE(Or);
          } else if (state.internalIndex == 1) {
            if (!exps.back()) ACP_NOT_OK;

            if (!expect(TokenType::QuestionMark)) ACP_EXP(exps.back().item);

            auto cond = std::make_shared<AST::ConditionalExpression>();
            cond->test = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);

            state.internalIndex = 2;
            ruleNode = std::move(cond);

            ACP_RULE(Expression);
          } else if (state.internalIndex == 2) {
            if (!exps.back()) ACP_NOT_OK;

            if (!expect(TokenType::Colon)) ACP_NOT_OK;

            auto cond = std::dynamic_pointer_cast<AST::ConditionalExpression>(ruleNode);
            cond->primaryResult = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);

            state.internalIndex = 3;

            ACP_RULE(PunctualConditonalExpression);
          } else if (state.internalIndex == 3) {
            if (!exps.back()) ACP_NOT_OK;

            auto cond = std::dynamic_pointer_cast<AST::ConditionalExpression>(ruleNode);
            cond->secondaryResult = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);

            ACP_NODE((cond));
          }
        } else if (rule == RuleType::NonequalityRelationalOperation) {
          if (expectBinaryOperation(rule, RuleType::Instanceof, {
            TokenType::OpeningAngleBracket,
            TokenType::ClosingAngleBracket,
            TokenType::LessThanOrEqualTo,
            TokenType::GreaterThanOrEqualTo,
          }, {
            AST::OperatorType::LessThan,
            AST::OperatorType::GreaterThan,
            AST::OperatorType::LessThanOrEqualTo,
            AST::OperatorType::GreaterThanOrEqualTo,
          }, state, exps, ruleNode, next, saveState, restoreState)) {
            continue;
          }
        } else if (rule == RuleType::EqualityRelationalOperation) {
          if (expectBinaryOperation(rule, RuleType::NonequalityRelationalOperation, {
            TokenType::Equality,
            TokenType::Inequality,
          }, {
            AST::OperatorType::EqualTo,
            AST::OperatorType::NotEqualTo,
          }, state, exps, ruleNode, next, saveState, restoreState)) {
            continue;
          }
        } else if (rule == RuleType::GroupedExpression) {
          if (state.internalIndex == 0) {
            if (!expect(TokenType::OpeningParenthesis)) ACP_NOT_OK;
            state.internalIndex = 1;
            ACP_RULE(Expression);
          } else {
            if (!exps.back()) ACP_NOT_OK;
            if (!expect(TokenType::ClosingParenthesis)) ACP_NOT_OK;
            ACP_EXP(exps.back().item);
          }
        } else if (rule == RuleType::ClassDefinition) {
          if (state.internalIndex == 0) {
            auto mods = expectModifiers(ModifierTargetType::Class);
            if (!expectKeyword("class")) ACP_NOT_OK;

            auto id = expect(TokenType::Identifier);
            if (!id) ACP_NOT_OK;

            auto def = std::make_shared<AST::ClassDefinitionNode>(id.raw);
            def->modifiers = mods;

            if (expect(TokenType::OpeningAngleBracket)) {
              state.internalIndex = 2;
              ruleNode = std::move(def);
              ACP_RULE(Generic);
            }

            if (expectKeyword("extends")) {
              bool dot = false;
              while (auto pid = expect(TokenType::Identifier)) {
                if (dot) {
                  auto& back = def->parents.back();
                  back = std::make_shared<AST::Accessor>(back, pid.raw);
                } else {
                  def->parents.push_back(std::make_shared<AST::Fetch>(pid.raw));
                }
                if (expect(TokenType::Dot)) {
                  dot = true;
                } else if (!expect(TokenType::Comma)) {
                  break;
                }
              }
              if (dot) ACP_NOT_OK;
            }

            if (!expect(TokenType::OpeningBrace)) ACP_NOT_OK;

            ruleNode = std::move(def);
            state.internalIndex = 1;
            ACP_RULE(ClassStatement);
          } else if (state.internalIndex == 2) {
            auto klass = std::dynamic_pointer_cast<AST::ClassDefinitionNode>(ruleNode);

            if (exps.back()) {
              klass->generics.push_back(std::dynamic_pointer_cast<AST::Generic>(*exps.back().item));
            } else if (klass->generics.size() > 0) {
              ACP_NOT_OK;
            }

            if (!expect(TokenType::Comma)) {
              state.internalIndex = 1;
              if (!expect(TokenType::ClosingAngleBracket)) ACP_NOT_OK;
              if (!expect(TokenType::OpeningBrace)) ACP_NOT_OK;
              ACP_RULE(ClassStatement);
            } else if (exps.back()) {
              ACP_RULE(Generic);
            } else {
              ACP_NOT_OK;
            }
          } else {
            auto klass = std::dynamic_pointer_cast<AST::ClassDefinitionNode>(ruleNode);

            if (exps.back()) {
              klass->statements.push_back(std::dynamic_pointer_cast<AST::ClassStatementNode>(*exps.back().item));
              ACP_RULE(ClassStatement);
            }

            if (!expect(TokenType::ClosingBrace)) ACP_NOT_OK;

            ACP_NODE((klass));
          }
        } else if (rule == RuleType::ClassStatement) {
          if (state.iteration == 0) {
            ACP_RULE_LIST(
              RuleType::ClassMember,
              RuleType::ClassSpecialMethod,
              RuleType::ClassMethod
            );
          }

          if (!exps.back()) ACP_NOT_OK;

          while (expect(TokenType::Semicolon)); // optional

          ACP_EXP(exps.back().item);
        } else if (rule == RuleType::ClassMember) {
          if (state.internalIndex == 0) {
            auto visibilityMod = expectModifier(ModifierTargetType::ClassStatement);
            if (!visibilityMod) ACP_NOT_OK;

            ruleNode = std::make_shared<AST::ClassMemberDefinitionStatement>(AST::parseVisibility(*visibilityMod));
            state.internalIndex = 1;
            ACP_RULE(VariableDefinition);
          } else {
            if (!exps.back()) ACP_NOT_OK;

            auto memberDef = std::dynamic_pointer_cast<AST::ClassMemberDefinitionStatement>(ruleNode);
            memberDef->varDef = std::dynamic_pointer_cast<AST::VariableDefinitionExpression>(*exps.back().item);
            if (!memberDef->varDef) ACP_NOT_OK;

            ACP_NODE((memberDef));
          }
        } else if (rule == RuleType::ClassMethod) {
          if (state.internalIndex == 0) {
            state.internalIndex = 1;
            ACP_RULE(Attribute);
          } else if (state.internalIndex == 1) {
            if (exps.back()) {
              ACP_RULE(Attribute);
            }

            exps.pop_back();

            auto visibilityMod = expectModifier(ModifierTargetType::ClassStatement);
            if (!visibilityMod) ACP_NOT_OK;

            std::vector<std::shared_ptr<AST::AttributeNode>> attrs;

            for (auto& exp: exps) {
              attrs.push_back(std::dynamic_pointer_cast<AST::AttributeNode>(*exp.item));
            }

            state.internalValue = attrs;
            ruleNode = std::make_shared<AST::ClassMethodDefinitionStatement>(AST::parseVisibility(*visibilityMod));
            state.internalIndex = 2;
            inClass = true;
            ACP_RULE(FunctionDefinition);
          } else {
            inClass = false;
            if (!exps.back()) ACP_NOT_OK;

            auto attrs = ALTACORE_ANY_CAST<std::vector<std::shared_ptr<AST::AttributeNode>>>(state.internalValue);
            auto methodDef = std::dynamic_pointer_cast<AST::ClassMethodDefinitionStatement>(ruleNode);
            methodDef->funcDef = std::dynamic_pointer_cast<AST::FunctionDefinitionNode>(*exps.back().item);
            if (!methodDef->funcDef) ACP_NOT_OK;

            methodDef->funcDef->attributes.insert(methodDef->funcDef->attributes.begin(), attrs.begin(), attrs.end());

            ACP_NODE((methodDef));
          }
        } else if (rule == RuleType::ClassSpecialMethod) {
          if (state.internalIndex == 0) {
            state.internalIndex = 1;
            ACP_RULE(Attribute);
          } else if (state.internalIndex == 1) {
            if (exps.back()) {
              ACP_RULE(Attribute);
            } else {
              exps.pop_back();
            }

            auto visibilityMod = expectModifier(ModifierTargetType::ClassStatement);
            if (!visibilityMod) ACP_NOT_OK;

            auto method = std::make_shared<AST::ClassSpecialMethodDefinitionStatement>(AST::parseVisibility(*visibilityMod), AST::SpecialClassMethod::Constructor);

            ruleNode = std::move(method);
            state.internalIndex = 2;

            ACP_RULE(Attribute);
          } else if (state.internalIndex == 2) {
            if (exps.back()) {
              ACP_RULE(Attribute);
            } else {
              exps.pop_back();
            }

            auto method = std::dynamic_pointer_cast<AST::ClassSpecialMethodDefinitionStatement>(ruleNode);

            for (auto& item: exps) {
              method->attributes.push_back(std::dynamic_pointer_cast<AST::AttributeNode>(*item.item));
            }

            auto kind = AST::SpecialClassMethod::Constructor;
            if (expectKeyword("constructor")) {
              kind = AST::SpecialClassMethod::Constructor;
            } else if (expectKeyword("destructor")) {
              kind = AST::SpecialClassMethod::Destructor;
            } else {
              ACP_NOT_OK;
            }

            method->type = kind;

            if (!expect(TokenType::OpeningParenthesis)) ACP_NOT_OK;
            
            state.internalIndex = 3;
            ACP_RULE(Parameter);
          } else if (state.internalIndex == 3) {
            auto method = std::dynamic_pointer_cast<AST::ClassSpecialMethodDefinitionStatement>(ruleNode);

            if (exps.back()) {
              method->parameters.push_back(std::dynamic_pointer_cast<AST::Parameter>(*exps.back().item));
              if (expect(TokenType::Comma)) {
                ACP_RULE(Parameter);
              }
            }

            if (!expect(TokenType::ClosingParenthesis)) ACP_NOT_OK;

            state.internalIndex = 4;
            inClass = true;
            ACP_RULE(Block);
          } else {
            inClass = false;
            if (!exps.back()) ACP_NOT_OK;

            auto method = std::dynamic_pointer_cast<AST::ClassSpecialMethodDefinitionStatement>(ruleNode);
            method->body = std::dynamic_pointer_cast<AST::BlockNode>(*exps.back().item);

            ACP_NODE((method));
          }
        } else if (rule == RuleType::ClassInstantiation) {
          if (state.internalIndex == 0) {
            saveState();
            if (!expectKeyword("new")) {
              state.internalIndex = 4;
              if (inClass) {
                state.internalIndex = 5;
                ACP_RULE(SuperClassFetch);
              }
              ACP_RULE_LIST(
                RuleType::Sizeof,
                RuleType::BooleanLiteral,
                RuleType::IntegralLiteral,
                RuleType::String,
                RuleType::Character,
                RuleType::Accessor,
                RuleType::DecimalLiteral
              );
            }
            state.internalIndex = 1;
            ACP_RULE(Accessor);
          } else if (state.internalIndex == 1 || state.internalIndex == 5) {
            if (!exps.back()) {
              if (state.internalIndex != 5) {
                restoreState();
              }
              state.internalIndex = 4;
              ACP_RULE_LIST(
                RuleType::Sizeof,
                RuleType::BooleanLiteral,
                RuleType::IntegralLiteral,
                RuleType::String,
                RuleType::Character,
                RuleType::Accessor,
                RuleType::DecimalLiteral
              );
            }

            auto inst = std::make_shared<AST::ClassInstantiationExpression>();

            inst->target = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);
            bool isSuperclassFetch = inst->target->nodeType() == AST::NodeType::SuperClassFetch;

            if (expect(TokenType::OpeningParenthesis)) {
              auto tmpState = currentState;
              auto name = expect(TokenType::Identifier);
              if (name && !expect(TokenType::Colon)) {
                name = Token(); // constructs an invalid expectation
                currentState = tmpState;
              }
              inst->arguments.push_back({
                (name) ? name.raw : "",
                nullptr,
              });

              state.internalIndex = 2;
              ruleNode = std::move(inst);
              ACP_RULE(Expression);
            } else if (isSuperclassFetch) {
              restoreState();
              state.internalIndex = 4;
              ACP_RULE_LIST(
                RuleType::Sizeof,
                RuleType::BooleanLiteral,
                RuleType::IntegralLiteral,
                RuleType::String,
                RuleType::Character,
                RuleType::Accessor,
                RuleType::DecimalLiteral
              );
            }

            ACP_NODE((inst));
          } else if (state.internalIndex == 2) {
            auto inst = std::dynamic_pointer_cast<AST::ClassInstantiationExpression>(ruleNode);

            if (exps.back()) {
              inst->arguments.back().second = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);
              if (expect(TokenType::Comma)) {
                auto tmpState = currentState;
                auto name = expect(TokenType::Identifier);
                if (name && !expect(TokenType::Colon)) {
                  name = Token(); // constructs an invalid expectation
                  currentState = tmpState;
                }

                inst->arguments.push_back({
                  (name) ? name.raw : "",
                  nullptr,
                });

                ACP_RULE(Expression);
              }
            } else {
              inst->arguments.pop_back();
            }

            if (!expect(TokenType::ClosingParenthesis)) ACP_NOT_OK;

            ACP_NODE((inst));
          } else if (state.internalIndex == 4) {
            ACP_EXP(exps.back().item);
          }
        } else if (rule == RuleType::Cast) {
          if (state.internalIndex == 0) {
            state.internalIndex = 1;
            ACP_RULE(NotOrPointerOrDereferenceOrPreIncDecOrPlusMinusOrBitNot);
          } else if (state.internalIndex == 1) {
            if (!exps.back()) ACP_NOT_OK;
            auto cast = std::make_shared<AST::CastExpression>();
            cast->target = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);
            ruleNode = std::move(cast);
            saveState();
            state.internalIndex = 2;
            if (!expectKeyword("as")) ACP_EXP(exps.back().item);
            ACP_RULE(Type);
          } else {
            auto cast = std::dynamic_pointer_cast<AST::CastExpression>(ruleNode);
            if (!exps.back()) {
              restoreState();
              ACP_NODE((cast->target));
            }
            cast->type = std::dynamic_pointer_cast<AST::Type>(*exps.back().item);
            ACP_NODE((cast));
          }
        } else if (rule == RuleType::NotOrPointerOrDereferenceOrPreIncDecOrPlusMinusOrBitNot) {
          if (state.internalIndex == 0) {
            saveState();

            std::stack<std::shared_ptr<AST::ExpressionNode>> exprs;
            bool isNot = false;
            bool isPointer = false;
            bool isDereference = false;
            bool isPreIncrement = false;
            bool isPreDecrement = false;
            bool isPlus = false;
            bool isMinus = false;
            bool isBitwiseNot = false;

            while (
              (isNot =          expect(TokenType::ExclamationMark) || expectKeyword("not")) ||
              (isPointer =      expect(TokenType::Ampersand)       || expectKeyword("getptr")) ||
              (isDereference =  expect(TokenType::Asterisk)        || expectKeyword("valueof")) ||
              (isPreIncrement = !!expect(TokenType::Increment)) ||
              (isPreDecrement = !!expect(TokenType::Decrement)) ||
              (isPlus =         !!expect(TokenType::PlusSign)) ||
              (isMinus =        !!expect(TokenType::MinusSign)) ||
              (isBitwiseNot =   !!expect(TokenType::Tilde))
            ) {
              if (isPointer) {
                exprs.push(std::make_shared<AST::PointerExpression>());
              } else if (isDereference) {
                exprs.push(std::make_shared<AST::DereferenceExpression>());
              } else {
                auto expr = std::make_shared<AST::UnaryOperation>();
                if (isNot) {
                  expr->type = AST::UOperatorType::Not;
                } else if (isPreIncrement) {
                  expr->type = AST::UOperatorType::PreIncrement;
                } else if (isPreDecrement) {
                  expr->type = AST::UOperatorType::PreDecrement;
                } else if (isPlus) {
                  expr->type = AST::UOperatorType::Plus;
                } else if (isMinus) {
                  expr->type = AST::UOperatorType::Minus;
                } else if (isBitwiseNot) {
                  expr->type = AST::UOperatorType::BitwiseNot;
                }
                exprs.push(expr);
              }
              isNot = isPointer = isDereference = isPreDecrement = isPreIncrement = isPlus = isMinus = isBitwiseNot = false;
            }

            state.internalValue = exprs;

            state.internalIndex = 1;
            ACP_RULE(FunctionCallOrSubscriptOrAccessorOrPostIncDec);
          } else {
            if (!exps.back()) ACP_NOT_OK;

            auto tgt = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);
            auto exprs = ALTACORE_ANY_CAST<std::stack<std::shared_ptr<AST::ExpressionNode>>>(state.internalValue);

            while (exprs.size() > 0) {
              auto parent = exprs.top();
              exprs.pop();
              if (auto unary = std::dynamic_pointer_cast<AST::UnaryOperation>(parent)) {
                unary->target = tgt;
              } else if (auto ptr = std::dynamic_pointer_cast<AST::PointerExpression>(parent)) {
                ptr->target = tgt;
              } else if (auto deref = std::dynamic_pointer_cast<AST::DereferenceExpression>(parent)) {
                deref->target = tgt;
              } else {
                throw std::runtime_error("that's weird... this should never happen (not unary, not pointer, not dereference)");
              }
              tgt = parent;
            }

            ACP_NODE(tgt);
          }
        } else if (rule == RuleType::WhileLoop) {
          if (state.internalIndex == 0) {
            if (!expectKeyword("while")) ACP_NOT_OK;

            state.internalIndex = 1;
            ACP_RULE(Expression);
          } else if (state.internalIndex == 1) {
            if (!exps.back()) ACP_NOT_OK;

            auto loop = std::make_shared<AST::WhileLoopStatement>();
            loop->test = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);
            
            ruleNode = std::move(loop);
            state.internalIndex = 2;
            ACP_RULE(Statement);
          } else {
            if (!exps.back()) ACP_NOT_OK;

            auto loop = std::dynamic_pointer_cast<AST::WhileLoopStatement>(ruleNode);
            loop->body = std::dynamic_pointer_cast<AST::StatementNode>(*exps.back().item);

            ACP_NODE((loop));
          }
        } else if (rule == RuleType::TypeAlias) {
          if (state.internalIndex == 0) {
            auto mods = expectModifiers(ModifierTargetType::TypeAlias);
            if (!expectKeyword("type")) ACP_NOT_OK;
            auto name = expect(TokenType::Identifier);
            if (!name) ACP_NOT_OK;
            if (!expect(TokenType::EqualSign)) ACP_NOT_OK;
            auto typeAlias = std::make_shared<AST::TypeAliasStatement>();
            typeAlias->modifiers = mods;
            typeAlias->name = name.raw;
            ruleNode = std::move(typeAlias);
            state.internalIndex = 1;
            ACP_RULE(Type);
          } else {
            if (!exps.back()) ACP_NOT_OK;
            auto typeAlias = std::dynamic_pointer_cast<AST::TypeAliasStatement>(ruleNode);
            typeAlias->type = std::dynamic_pointer_cast<AST::Type>(*exps.back().item);
            ACP_NODE((typeAlias));
          }
        } else if (rule == RuleType::SuperClassFetch) {
          if (state.internalIndex == 0) {
            if (!expectKeyword("super")) ACP_NOT_OK;
            auto sup = std::make_shared<AST::SuperClassFetch>();
            if (expect(TokenType::OpeningAngleBracket)) {
              if (auto lit = expect(TokenType::Integer)) {
                sup->fetch = std::make_shared<AST::IntegerLiteralNode>(lit.raw);
                if (!expect(TokenType::ClosingAngleBracket)) ACP_NOT_OK;
              } else {
                state.internalIndex = 1;
                ruleNode = std::move(sup);
                ACP_RULE(StrictAccessor);
              }
            }

            ACP_NODE((sup));
          } else {
            auto sup = std::dynamic_pointer_cast<AST::SuperClassFetch>(ruleNode);

            if (!exps.back()) ACP_NOT_OK;
            sup->fetch = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);
            if (!expect(TokenType::ClosingAngleBracket)) ACP_NOT_OK;

            ACP_NODE((sup));
          }
        } else if (rule == RuleType::Instanceof) {
          if (state.internalIndex == 0) {
            state.internalIndex = 1;
            ACP_RULE(Shift);
          } else if (state.internalIndex == 1) {
            if (!exps.back()) ACP_NOT_OK;
            if (!expectKeyword("instanceof")) ACP_EXP(exps.back().item);
            auto instOf = std::make_shared<AST::InstanceofExpression>();
            instOf->target = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);
            ruleNode = std::move(instOf);
            state.internalIndex = 2;
            ACP_RULE(Type);
          } else {
            auto instOf = std::dynamic_pointer_cast<AST::InstanceofExpression>(ruleNode);
            if (!exps.back()) ACP_NOT_OK;
            instOf->type = std::dynamic_pointer_cast<AST::Type>(*exps.back().item);
            ACP_NODE((instOf));
          }
        } else if (rule == RuleType::Generic) {
          if (state.internalIndex == 0) {
            auto name = expect(TokenType::Identifier);
            if (!name) ACP_NOT_OK;
            auto node = std::make_shared<AST::Generic>(name.raw);
            ACP_NODE(node);
          }
        } else if (rule == RuleType::NullRule) {
          ACP_NOT_OK;
        } else if (rule == RuleType::ForLoop) {
          if (state.internalIndex == 0) {
            if (!expectKeyword("for")) ACP_NOT_OK;
            if (!expect(TokenType::OpeningParenthesis)) ACP_NOT_OK;

            state.internalIndex = 1;
            ACP_RULE(Expression);
          } else if (state.internalIndex == 1) {
            if (!expect(TokenType::Semicolon)) ACP_NOT_OK;

            ruleNode = std::make_shared<AST::ForLoopStatement>();
            auto loop = std::dynamic_pointer_cast<AST::ForLoopStatement>(ruleNode);
            if (exps.back()) {
              loop->initializer = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);
            }

            state.internalIndex = 2;
            ACP_RULE(Expression);
          } else if (state.internalIndex == 2) {
            if (!expect(TokenType::Semicolon)) ACP_NOT_OK;

            auto loop = std::dynamic_pointer_cast<AST::ForLoopStatement>(ruleNode);
            if (exps.back()) {
              loop->condition = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);
            }

            state.internalIndex = 3;
            ACP_RULE(Expression);
          } else if (state.internalIndex == 3) {
            auto loop = std::dynamic_pointer_cast<AST::ForLoopStatement>(ruleNode);
            if (exps.back()) {
              loop->increment = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);
            }

            if (!expect(TokenType::ClosingParenthesis)) ACP_NOT_OK;

            state.internalIndex = 4;
            ACP_RULE(Statement);
          } else if (state.internalIndex == 4) {
            if (!exps.back()) ACP_NOT_OK;

            auto loop = std::dynamic_pointer_cast<AST::ForLoopStatement>(ruleNode);
            loop->body = std::dynamic_pointer_cast<AST::StatementNode>(*exps.back().item);

            ACP_NODE(loop);
          }
        } else if (rule == RuleType::RangedFor) {
          if (state.internalIndex == 0) {
            if (!expectKeyword("for")) ACP_NOT_OK;

            auto loop = std::make_shared<AST::RangedForLoopStatement>();
            ruleNode = loop;

            auto name = expect(TokenType::Identifier);
            if (!name) ACP_NOT_OK;

            if (!expect(TokenType::Colon)) ACP_NOT_OK;

            loop->counterName = name.raw;

            state.internalIndex = 1;
            ACP_RULE(Type);
          } else if (state.internalIndex == 1) {
            if (!exps.back()) ACP_NOT_OK;

            auto loop = std::dynamic_pointer_cast<AST::RangedForLoopStatement>(ruleNode);
            loop->counterType = std::dynamic_pointer_cast<AST::Type>(*exps.back().item);

            if (!expectKeyword("in")) ACP_NOT_OK;

            state.internalIndex = 2;
            ACP_RULE(Expression);
          } else if (state.internalIndex == 2) {
            if (!exps.back()) ACP_NOT_OK;

            auto loop = std::dynamic_pointer_cast<AST::RangedForLoopStatement>(ruleNode);
            loop->start = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);

            if (expect(TokenType::Colon) && expect(TokenType::Colon)) {
              loop->decrement = true;
              if (expect(TokenType::Colon)) {
                loop->inclusive = true;
              }
            } else if (expect(TokenType::Dot) && expect(TokenType::Dot)) {
              if (expect(TokenType::Dot)) {
                loop->inclusive = true;
              }
            } else {
              ACP_NOT_OK;
            }

            state.internalIndex = 3;
            ACP_RULE(Expression);
          } else if (state.internalIndex == 3) {
            if (!exps.back()) ACP_NOT_OK;

            auto loop = std::dynamic_pointer_cast<AST::RangedForLoopStatement>(ruleNode);
            loop->end = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);

            state.internalIndex = 4;
            ACP_RULE(Statement);
          } else if (state.internalIndex == 4) {
            if (!exps.back()) ACP_NOT_OK;

            auto loop = std::dynamic_pointer_cast<AST::RangedForLoopStatement>(ruleNode);
            loop->body = std::dynamic_pointer_cast<AST::StatementNode>(*exps.back().item);

            ACP_NODE(loop);
          }
        } else if (rule == RuleType::Accessor) {
          if (state.internalIndex == 0) {
            state.internalIndex = 1;
            if (inClass) {
              ACP_RULE_LIST(
                RuleType::SuperClassFetch,
                RuleType::Fetch,
                RuleType::GroupedExpression,
                RuleType::StrictAccessor,
              );
            } else {
              ACP_RULE_LIST(
                RuleType::Fetch,
                RuleType::GroupedExpression,
                RuleType::StrictAccessor,
              );
            }
          } else {
            ACP_EXP(exps.back().item);
          }
        } else if (rule == RuleType::Sizeof) {
          if (state.internalIndex == 0) {
            if (!expectKeyword("sizeof")) ACP_NOT_OK;
            state.internalIndex = 1;
            ACP_RULE(Type);
          } else {
            if (!exps.back()) ACP_NOT_OK;
            auto op = std::make_shared<AST::SizeofOperation>();
            op->target = std::dynamic_pointer_cast<AST::Type>(*exps.back().item);
            ACP_NODE(op);
          }
        } else if (rule == RuleType::And) {
          if (expectBinaryOperation(RuleType::And, RuleType::BitwiseOr, {
            TokenType::And
          }, {
            AST::OperatorType::LogicalAnd,
          }, state, exps, ruleNode, next, saveState, restoreState)) {
            continue;
          }
        } else if (rule == RuleType::Or) {
          if (expectBinaryOperation(RuleType::Or, RuleType::And, {
            TokenType::Or
          }, {
            AST::OperatorType::LogicalOr,
          }, state, exps, ruleNode, next, saveState, restoreState)) {
            continue;
          }
        } else if (rule == RuleType::Shift) {
          if (expectBinaryOperation(RuleType::Shift, RuleType::AdditionOrSubtraction, {
            TokenType::LeftShift,
            TokenType::RightShift,
          }, {
            AST::OperatorType::LeftShift,
            AST::OperatorType::RightShift,
          }, state, exps, ruleNode, next, saveState, restoreState)) {
            continue;
          }
        } else if (rule == RuleType::BitwiseAnd) {
          if (expectBinaryOperation(RuleType::BitwiseAnd, RuleType::EqualityRelationalOperation, {
            TokenType::Ampersand,
          }, {
            AST::OperatorType::BitwiseAnd,
          }, state, exps, ruleNode, next, saveState, restoreState)) {
            continue;
          }
        } else if (rule == RuleType::BitwiseOr) {
          if (expectBinaryOperation(RuleType::BitwiseOr, RuleType::BitwiseXor, {
            TokenType::Pipe,
          }, {
            AST::OperatorType::BitwiseOr,
          }, state, exps, ruleNode, next, saveState, restoreState)) {
            continue;
          }
        } else if (rule == RuleType::BitwiseXor) {
          if (expectBinaryOperation(RuleType::BitwiseXor, RuleType::BitwiseAnd, {
            TokenType::Caret,
          }, {
            AST::OperatorType::BitwiseXor,
          }, state, exps, ruleNode, next, saveState, restoreState)) {
            continue;
          }
        } else if (rule == RuleType::DecimalLiteral) {
          auto decimal = expect(TokenType::Decimal);
          if (!decimal) ACP_NOT_OK;
          ACP_NODE(std::make_shared<AST::FloatingPointLiteralNode>(decimal.raw));
        } else if (rule == RuleType::Structure) {
          if (state.internalIndex == 0) {
            state.internalIndex = 1;
            ACP_RULE(Attribute);
          } else if (state.internalIndex == 1) {
            if (exps.back()) ACP_RULE(Attribute);

            exps.pop_back();

            auto structure = ruleNode ? std::dynamic_pointer_cast<AST::StructureDefinitionStatement>(ruleNode) : std::make_shared<AST::StructureDefinitionStatement>();
            auto mods = expectModifiers(ModifierTargetType::Structure);
            structure->modifiers.insert(structure->modifiers.end(), mods.begin(), mods.end());

            ruleNode = std::move(structure);
            state.internalIndex = 2;
            ACP_RULE(Attribute);
          } else if (state.internalIndex == 2) {
            if (exps.back()) {
              state.internalIndex = 1;
              ACP_RULE(Attribute);
            }

            exps.pop_back();

            auto structure = std::dynamic_pointer_cast<AST::StructureDefinitionStatement>(ruleNode);

            for (auto& exp: exps) {
              structure->attributes.push_back(std::dynamic_pointer_cast<AST::AttributeNode>(*exp.item));
            }
            exps.clear();

            if (!expectKeyword("struct")) ACP_NOT_OK;

            auto name = expect(TokenType::Identifier);
            if (!name) ACP_NOT_OK;
            structure->name = name.raw;

            if (!expect(TokenType::OpeningBrace)) ACP_NOT_OK;

            state.internalIndex = 3;
            ACP_RULE(NullRule);
          } else if (state.internalIndex == 3) {
            auto name = expect(TokenType::Identifier);
            if (!name) {
              state.internalIndex = 5;
              ACP_RULE(NullRule);
            }

            if (!expect(TokenType::Colon)) ACP_NOT_OK;

            auto structure = std::dynamic_pointer_cast<AST::StructureDefinitionStatement>(ruleNode);
            structure->members.push_back(std::make_pair(std::shared_ptr<AST::Type>(nullptr), name.raw));

            state.internalIndex = 4;
            ACP_RULE(Type);
          } else if (state.internalIndex == 4) {
            if (!exps.back()) ACP_NOT_OK;

            auto structure = std::dynamic_pointer_cast<AST::StructureDefinitionStatement>(ruleNode);
            structure->members.back().first = std::dynamic_pointer_cast<AST::Type>(*exps.back().item);

            // optional line terminators
            while (expect(TokenType::Semicolon));
            while (expect(TokenType::Comma));

            state.internalIndex = 3;
            ACP_RULE(NullRule);
          } else {
            if (!expect(TokenType::ClosingBrace)) ACP_NOT_OK;

            auto structure = std::dynamic_pointer_cast<AST::StructureDefinitionStatement>(ruleNode);

            ACP_NODE(structure);
          }
        } else if (rule == RuleType::Export) {
          if (state.internalIndex == 0) {
            if (!expectKeyword("export")) ACP_NOT_OK;

            auto statement = std::make_shared<AST::ExportStatement>();
            ruleNode = statement;

            if (expect(TokenType::Asterisk)) {
              statement->externalTarget = std::make_shared<AST::ImportStatement>();
              statement->externalTarget->isAliased = statement->externalTarget->isManual = true;
              bool foundAs = false;
              if (expectKeyword("as")) {
                foundAs = true;
                auto alias = expect(TokenType::Identifier);
                if (!alias) ACP_NOT_OK;
                statement->externalTarget->alias = alias.raw;
              }
              if (!expectKeyword("from")) ACP_NOT_OK;
              auto request = expect(TokenType::String);
              if (!request) ACP_NOT_OK;
              statement->externalTarget->request = request.raw.substr(1, request.raw.length() - 2);
              if (!foundAs && expectKeyword("as")) {
                foundAs = true;
                auto alias = expect(TokenType::Identifier);
                if (!alias) ACP_NOT_OK;
                statement->externalTarget->alias = alias.raw;
              }
              ACP_NODE(ruleNode);
            }

            state.internalValue = false;

            if (expect(TokenType::OpeningBrace)) {
              statement->externalTarget = std::make_shared<AST::ImportStatement>();
              state.internalIndex = 2;
              state.internalValue = true;
              ACP_RULE(NullRule);
            }

            saveState();

            state.internalIndex = 1;
            ACP_RULE(StrictAccessor);
          } else if (state.internalIndex == 1) {
            if (!exps.back()) ACP_NOT_OK;

            auto statement = std::dynamic_pointer_cast<AST::ExportStatement>(ruleNode);

            if (expect(TokenType::Comma)) {
              restoreState();
              statement->externalTarget = std::make_shared<AST::ImportStatement>();
              state.internalIndex = 2;
              ACP_RULE(NullRule);
            }

            Token alias;
            if (expectKeyword("as")) {
              alias = expect(TokenType::Identifier);
            }

            if (expectKeyword("from") && expect(TokenType::String)) {
              restoreState();
              statement->externalTarget = std::make_shared<AST::ImportStatement>();
              state.internalIndex = 2;
              ACP_RULE(NullRule);
            }

            statement->localTarget = std::dynamic_pointer_cast<AST::RetrievalNode>(*exps.back().item);
            statement->localTargetAlias = alias ? alias.raw : "";

            ACP_NODE(statement);
          } else if (state.internalIndex == 2) {
            Token id;

            auto statement = std::dynamic_pointer_cast<AST::ExportStatement>(ruleNode);

            while (id = expect(TokenType::Identifier)) {
              statement->externalTarget->imports.push_back(std::make_pair(id.raw, ""));
              saveState();
              if (expectKeyword("as")) {
                auto alias = expect(TokenType::Identifier);
                if (!alias) {
                  restoreState();
                } else {
                  statement->externalTarget->imports.back().second = alias.raw;
                }
              }
              if (!expect(TokenType::Comma)) break;
            }

            if (ALTACORE_ANY_CAST<bool>(state.internalValue)) {
              if (!expect(TokenType::ClosingBrace)) ACP_NOT_OK;
            }

            if (!expectKeyword("from")) ACP_NOT_OK;

            auto request = expect(TokenType::String);
            if (!request) ACP_NOT_OK;
            statement->externalTarget->request = request.raw.substr(1, request.raw.length() - 2);

            ACP_NODE(statement);
          }
        }

        next();
      }

      
      Timing::parseTimes[absoluteFilePath].stop();
    };

    #undef ACP_NOT_OK
    #undef ACP_NODE
    #undef ACP_RULES
    #undef ACP_RULE_LIST
    #undef ACP_EXP
    #undef ACP_RULE
  };
};
