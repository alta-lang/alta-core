#include <algorithm>
#include "../include/altacore/parser.hpp"
#include "../include/altacore/util.hpp"
#include "../include/altacore/logging.hpp"

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
        if (relexer.tokens.size() > 0 && relexer.tokens.back().type == TokenType::PreprocessorDirective && relexer.tokens.back().raw == "#") {
          relexer.tokens.pop_back();
        }
        Timing::parseTimes[filePath].start();
        tokens = relexer.tokens;
      }

      for (auto& expectation: expectations) {
        if (tokens[currentState.currentPosition].type == expectation) {
          tok = tokens[currentState.currentPosition++];
          break;
        }
      }

      if (tok.firstInLine && tok.type == TokenType::OpeningParenthesis) {
        if (currentState.currentPosition - 1 != 0) {
          auto prev = tokens[currentState.currentPosition - 2];
          if (prev.type == TokenType::Integer || prev.type == TokenType::Identifier || prev.type == TokenType::String || prev.type == TokenType::ClosingParenthesis || prev.type == TokenType::ClosingAngleBracket) {
            if (findingConditionalTest) {
              Logging::log(Logging::Message("parser", "S0001", Logging::Severity::Warning, Errors::Position(tok.line, tok.column, filePath, tok.position), "To prevent a possible error and silence this warning, surround the conditional's body with braces ({...})"));
            } else {
              Logging::log(Logging::Message("parser", "S0001", Logging::Severity::Warning, Errors::Position(tok.line, tok.column, filePath, tok.position), "To prevent a possible error and silence this warning, add a semicolon (;) before the parenthesis"));
            }
          }
        } else {
          Logging::log(Logging::Message("parser", "S0001", Logging::Severity::Warning, Errors::Position(tok.line, tok.column, filePath, tok.position)));
        }
      }

      if (tok)
        currentState.lastToken = tok;

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

    bool Parser::expectBinaryOperation(RuleType rule, RuleType nextHigherPrecedentRule, std::vector<std::vector<TokenType>> operatorTokens, std::vector<AST::OperatorType> operatorTypes, RuleState& state, std::vector<Expectation>& exps, std::shared_ptr<AST::Node>& ruleNode, NextFunctionType next, SaveStateType saveState, RestoreStateType restoreState) {
      if (operatorTokens.size() != operatorTypes.size()) {
        throw std::runtime_error("malformed binary operation expectation: the number of operator tokens must match the number of operator types.");
      }
      auto addPositionInformation = [&](std::shared_ptr<AST::Node> node) -> void {
        auto& tok = tokens[state.stateAtStart.currentPosition];
        node->position.line = tok.line;
        node->position.column = tok.column;
        node->position.filePosition = tok.position;
        node->position.file = filePath;
      };
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
        addPositionInformation(binOp);
        binOp->left = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);

        size_t idx = SIZE_MAX;
        for (size_t i = 0; i < operatorTokens.size(); i++) {
          auto& opToks = operatorTokens[i];
          auto exp = expectSequence(opToks, true);
          if (exp.size() == opToks.size()) {
            idx = i;
            break;
          }
        }
        if (idx == SIZE_MAX) {
          if (exps.back().item) {
            next(true, {}, *exps.back().item);
          } else {
            next(false, {}, nullptr);
          }
          return true;
        }

        binOp->type = operatorTypes[idx];

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

        size_t idx = SIZE_MAX;
        for (size_t i = 0; i < operatorTokens.size(); i++) {
          auto& opToks = operatorTokens[i];
          auto exp = expectSequence(opToks, true);
          if (exp.size() == opToks.size()) {
            idx = i;
            break;
          }
        }

        if (idx != SIZE_MAX) {
          auto otherBinOp = std::make_shared<AST::BinaryOperation>();
          addPositionInformation(otherBinOp);
          otherBinOp->left = binOp;
          otherBinOp->type = operatorTypes[idx];

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
    
    std::vector<Token> Parser::expectSequence(std::vector<TokenType> expectations, bool exact) {
      std::vector<Token> tokens;
      auto savedState = currentState;

      for (auto& exp: expectations) {
        auto tok = expect(exp);
        if (!tok) {
          currentState = savedState;
          return {};
        }
        if (tokens.size() > 0) {
          if (exact && tok.column != tokens.back().column + 1) {
            currentState = savedState;
            return {};
          }
        }
        tokens.push_back(tok);
      }

      return tokens;
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
        RuleState(currentState, RuleType::None),
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
            RuleState(currentState, RuleType::None),
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
            PREPO_RULE(Not);
          } else if (state.internalIndex == 1) {
            if (!exps.back()) PREPO_NOT_OK;

            if (!expect(TokenType::Equality)) PREPO_EXP(exps.back().item);

            state.internalIndex = 2;
            PREPO_RULE(Not);
          } else {
            if (!exps.back()) PREPO_NOT_OK;

            auto left = (exps.size() == 1) ? ruleNode : *exps.front().item;
            auto result = evaluateExpressions ? PrepoExpression(left == *exps.back().item) : PrepoExpression();

            exps.clear();

            if (expect(TokenType::Equality)) {
              ruleNode = std::move(result);
              PREPO_RULE(Not);
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
        } else if (rule == PrepoRuleType::Not) {
          if (state.internalIndex == 0) {
            if (expect(TokenType::ExclamationMark) || expectKeyword("not")) {
              state.internalIndex = 1;
            } else {
              state.internalIndex = 2;
            }
            PREPO_RULE(MacroCall);
          } else {
            if (state.internalIndex == 1) {
              if (!exps.back()) PREPO_NOT_OK;
              PREPO_NODE(!*exps.back().item);
            } else {
              PREPO_EXP(exps.back().item);
            }
          }
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

    struct NodeFactory {
      std::function<void(std::shared_ptr<AST::Node>)> _addPositionInformation;

      NodeFactory(std::function<void(std::shared_ptr<AST::Node>)> addPositionInformation):
        _addPositionInformation(addPositionInformation)
        {};

      template<typename T, typename... Args>
      std::shared_ptr<T> create(Args&&... args) {
        auto node = std::make_shared<T>(std::forward<Args>(args)...);
        _addPositionInformation(node);
        return node;
      };
    };

    void Parser::parse() {
      auto absoluteFilePath = filePath.absolutify();
      Timing::parseTimes[absoluteFilePath].start();
      std::stack<RuleStackElement> ruleStack;

      ruleStack.emplace(
        RuleType::Root,
        std::stack<RuleType>(),
        RuleState(currentState, RuleType::Root),
        std::vector<Expectation>(),
        nullptr,
        std::make_tuple(currentState, std::deque<bool>(), std::deque<bool>(), true)
      );

      auto addPositionInformation = [&](std::shared_ptr<AST::Node> node) -> void {
        auto& state = std::get<2>(ruleStack.top());
        auto& tok = (state.stateAtStart.currentPosition >= tokens.size())
                      ? tokens.back()
                      : tokens[state.stateAtStart.currentPosition];
        node->position.line = tok.line;
        node->position.column = tok.column;
        node->position.filePosition = tok.position;
        node->position.file = filePath;
      };

      auto nodeFactory = NodeFactory(addPositionInformation);

      auto next = [&](bool ok = false, std::vector<RuleType> rules = {}, NodeType result = nullptr) {
        auto& state = std::get<2>(ruleStack.top());
        state.iteration++;

        if (ok && state.currentState.currentPosition > farthestRule.currentState.currentPosition) {
          farthestRule = state;
        }

        if (result) {
          addPositionInformation(result);

          if (auto statement = std::dynamic_pointer_cast<AST::ExportStatement>(result)) {
            if (statement->externalTarget) {
              addPositionInformation(statement->externalTarget);
            }
          }
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

      std::deque<bool> prepoLevels;
      std::deque<bool> prepoLast;
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

      using SavedState = std::tuple<State, std::deque<bool>, std::deque<bool>, bool>;

      auto manualSaveSpecificState = [&](State state) -> SavedState {
        return std::make_tuple(state, prepoLevels, prepoLast, advanceExp);
      };
      auto manualSaveState = [&]() {
        return manualSaveSpecificState(currentState);
      };
      auto manualRestoreState = [&](SavedState savedState) {
        auto [state, levels, last, advance] = savedState;
        currentState = state;
        prepoLevels = levels;
        prepoLast = last;
        advanceExp = advance;
      };

      auto topLevelTrue = [&]() {
        if (prepoLevels.size() < 1) return true;
        if (prepoLevels.back()) return true;
        return false;
      };

      auto foundBlock = [&]() {
        if (prepoLast.size() < 1) return true;
        if (prepoLast.back()) return true;
        return false;
      };

      auto foundParentBlock = [&]() {
        if (prepoLast.size() < 2) return true;
        if (prepoLast[prepoLast.size() - 2]) return true;
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
            RuleState(currentState, rule),
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
            if (!foundBlock()) {
              prepoLevels.push_back(false);
              prepoLast.push_back(false);
            } else {
              auto expr = expectPrepoExpression();
              if (!expr) {
                ignoreLine();
              } else {
                prepoLevels.push_back(!!*expr);
                prepoLast.push_back(!!*expr);
              }
            }
          } else if (directive == "else") {
            if (!foundParentBlock()) {
              prepoLast.back() = false;
            } else {
              auto ifTok = peek();
              if (ifTok.line == currentLine && ifTok.raw == "if") {
                expectAnyToken(); // consume the "if"
                if (topLevelTrue()) {
                  prepoLast.back() = false;
                } else {
                  auto expr = expectPrepoExpression();
                  if (!expr) {
                    ignoreLine();
                  } else {
                    prepoLevels.back() = !!*expr;
                    prepoLast.back() = !!*expr;
                  }
                }
              } else {
                if (topLevelTrue()) {
                  prepoLast.back() = false;
                } else {
                  prepoLevels.back() = true;
                  prepoLast.back() = true;
                }
              }
            }
          } else if (directive == "end") {
            auto nextTok = peek();
            if (nextTok.line == currentLine && nextTok.raw == "if") {
              expectAnyToken(); // consume the "if"
              prepoLevels.pop_back();
              prepoLast.pop_back();
            } else {
              ignoreLine();
            }
          } else if (foundBlock()) {
            if (directive == "define") {
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
          // logic for the initial call
          if (state.iteration == 0) ACP_RULE(ModuleOnlyStatement);

          // basically a while loop that continues as long statements are available
          if (exps.back()) ACP_RULE(ModuleOnlyStatement);

          exps.pop_back(); // remove the last (implicitly invalid) expectation

          std::vector<std::shared_ptr<AST::StatementNode>> statements;
          for (auto& exp: exps) {
            auto stmt = std::dynamic_pointer_cast<AST::StatementNode>(*exp.item);
            if (stmt == nullptr) throw std::runtime_error("AST node given was not of the expected type");
            statements.push_back(stmt);
          }

          if (currentState.currentPosition < tokens.size()) {
            auto& tok = tokens[farthestRule.currentState.currentPosition];
            auto pos = Errors::Position(tok.line, tok.column, filePath);
            pos.filePosition = tok.position;
            throw Errors::ParsingError("input not completely parsed; assuming failure", pos);
          }

          root = nodeFactory.create<AST::RootNode>(statements);

          next(true);
          break;
        } else if (rule == RuleType::Statement) {
          if (state.iteration == 0) {
            while (expect(TokenType::Semicolon)); // optional

            ACP_RULE_LIST(
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
              RuleType::VariableDeclaration,
              RuleType::Alias,
              RuleType::Delete,
              RuleType::ControlDirective,
              RuleType::TryCatch,
              RuleType::Throw,
              RuleType::CodeLiteral,
              RuleType::Bitfield,
              RuleType::Assertion,
              RuleType::Expression,

              // general attributes must come last because
              // they're supposed to be able to interpreted as part of
              // other statements that accept attributes if any such
              // statement is present
              RuleType::GeneralAttribute,
            );
          }

          if (!exps.back()) ACP_NOT_OK;

          while (expect(TokenType::Semicolon)); // optional

          auto& exp = exps.back();
          auto ret = std::dynamic_pointer_cast<AST::StatementNode>(*exp.item);
          if (exp.type == RuleType::Expression) {
            auto expr = std::dynamic_pointer_cast<AST::ExpressionNode>(*exp.item);
            if (expr == nullptr) throw std::runtime_error("wtf");
            ret = nodeFactory.create<AST::ExpressionStatement>(expr);
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

            auto funcDef = nodeFactory.create<AST::FunctionDefinitionNode>();
            funcDef->modifiers = expectModifiers(ModifierTargetType::Function);

            ruleNode = std::move(funcDef);
            state.internalIndex = 2;
            ACP_RULE(Attribute);
          } else if (state.internalIndex == 2) {
            if (exps.back()) ACP_RULE(Attribute);

            exps.pop_back();

            auto funcDef = std::dynamic_pointer_cast<AST::FunctionDefinitionNode>(ruleNode);
            auto tmpMods = expectModifiers(ModifierTargetType::Function);
            funcDef->modifiers.insert(funcDef->modifiers.end(), tmpMods.begin(), tmpMods.end());

            if (tmpMods.size() > 0) {
              ACP_RULE(Attribute);
            }

            for (auto& exp: exps) {
              funcDef->attributes.push_back(std::dynamic_pointer_cast<AST::AttributeNode>(*exp.item));
            }
            exps.clear();

            if (!expectKeyword("function")) ACP_NOT_OK;

            funcDef->isGenerator = std::find(funcDef->modifiers.begin(), funcDef->modifiers.end(), "generator") != funcDef->modifiers.end();
            funcDef->isAsync = std::find(funcDef->modifiers.begin(), funcDef->modifiers.end(), "async") != funcDef->modifiers.end();

            auto name = expect(TokenType::Identifier);
            if (!name) ACP_NOT_OK;
            funcDef->name = name.raw;
            
            if (expect(TokenType::OpeningAngleBracket)) {
              while (auto generic = expect(TokenType::Identifier)) {
                funcDef->generics.push_back(nodeFactory.create<AST::Generic>(generic.raw));
                if (!expect(TokenType::Comma)) break;
              }
              if (funcDef->generics.size() < 1) ACP_NOT_OK;
              if (!expect(TokenType::ClosingAngleBracket)) ACP_NOT_OK;
            }

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
            if (funcDef->isGenerator && expect(TokenType::OpeningParenthesis)) {
              state.internalIndex = 6;
              ACP_RULE(Type);
            }
            if (!expect(TokenType::Colon)) ACP_NOT_OK;

            state.internalIndex = 4;
            ACP_RULE(Type);
          } else if (state.internalIndex == 4) {
            auto funcDef = std::dynamic_pointer_cast<AST::FunctionDefinitionNode>(ruleNode);

            if (!exps.back()) ACP_NOT_OK;
            funcDef->returnType = std::dynamic_pointer_cast<AST::Type>(*exps.back().item);

            state.internalIndex = 5;
            ACP_RULE(Block);
          } else if (state.internalIndex == 5) {
            auto funcDef = std::dynamic_pointer_cast<AST::FunctionDefinitionNode>(ruleNode);

            if (!exps.back()) ACP_NOT_OK;
            funcDef->body = std::dynamic_pointer_cast<AST::BlockNode>(*exps.back().item);

            ACP_NODE(std::move(funcDef));
          } else if (state.internalIndex == 6) {
            auto funcDef = std::dynamic_pointer_cast<AST::FunctionDefinitionNode>(ruleNode);

            if (exps.back()) {
              if (!(expect(TokenType::Dot) && expect(TokenType::Dot) && expect(TokenType::Dot))) {
                ACP_NOT_OK;
              }
              funcDef->generatorParameter = std::dynamic_pointer_cast<AST::Type>(*exps.back().item);
            }

            if (!expect(TokenType::ClosingParenthesis)) ACP_NOT_OK;
            if (!expect(TokenType::Colon)) ACP_NOT_OK;

            state.internalIndex = 4;
            ACP_RULE(Type);
          }
        } else if (rule == RuleType::Parameter) {
          if (state.internalIndex == 0) {
            state.internalIndex = 1;
            ACP_RULE(Attribute);
          } else if (state.internalIndex == 1) {
            if (exps.back()) ACP_RULE(Attribute);

            exps.pop_back(); // remove the last (implicitly invalid) expectation

            auto param = nodeFactory.create<AST::Parameter>();

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
          } else if (state.internalIndex == 2) {
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
              param->type = nodeFactory.create<AST::Type>();
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

            if (expect(TokenType::EqualSign)) {
              state.internalIndex = 3;
              ruleNode = std::move(param);
              ACP_RULE(Expression);
            }

            ACP_NODE(std::move(param));
          } else {
            if (!exps.back()) ACP_NOT_OK;

            auto param = std::dynamic_pointer_cast<AST::Parameter>(ruleNode);

            param->defaultValue = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);

            ACP_NODE(std::move(param));
          }
        } else if (rule == RuleType::StrictAccessor) {
          if (state.internalIndex == 0) {
            state.internalIndex = 3;
            ACP_RULE(Attribute);
          } else if (state.internalIndex == 3) {
            if (exps.back()) ACP_RULE(Attribute);
            exps.pop_back();
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
            if (!expect(TokenType::Dot)) {
              auto expr = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);
              exps.pop_back();
              for (auto exp: exps) {
                expr->attributes.push_back(std::dynamic_pointer_cast<AST::AttributeNode>(*exp.item));
              }
              ACP_NODE(expr);
            }

            auto query = expect(TokenType::Identifier);
            if (!query) {
              currentState = savedState;
              auto expr = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);
              exps.pop_back();
              for (auto exp: exps) {
                expr->attributes.push_back(std::dynamic_pointer_cast<AST::AttributeNode>(*exp.item));
              }
              ACP_NODE(expr);
            }

            auto acc = nodeFactory.create<AST::Accessor>(std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item), query.raw);

            savedState = currentState;
            while (expect(TokenType::Dot)) {
              query = expect(TokenType::Identifier);
              if (!query) {
                currentState = savedState;
                break;
              }
              acc = nodeFactory.create<AST::Accessor>(acc, query.raw);
              savedState = currentState;
            }

            exps.pop_back();
            for (auto exp: exps) {
              acc->attributes.push_back(std::dynamic_pointer_cast<AST::AttributeNode>(*exp.item));
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
            auto type = nodeFactory.create<AST::Type>();

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
              // if we continue, we're parsing a raw function pointer type
              type->isFunction = true;

              state.internalIndex = 2;
              ACP_RULE(Type);
            } else if (expect(TokenType::FatReturns)) {
              // if we continue, we're parsing a function pointer type
              type->isFunction = true;
              type->isLambda = true;

              state.internalIndex = 2;
              ACP_RULE(Type);
            } else if (type->parameters.size() > 1) {
              // somehow, we detected parameters, but there's no return indicator,
              // so this isn't a type
              ACP_NOT_OK;
            } else {
              auto otherType = std::get<0>(type->parameters[0]);
              if (expect(TokenType::QuestionMark)) {
                auto newType = nodeFactory.create<AST::Type>();
                newType->modifiers = type->modifiers;
                newType->isOptional = true;
                newType->optionalTarget = otherType;
                ACP_NODE(newType);
              } else {
                otherType->modifiers.insert(otherType->modifiers.begin(), type->modifiers.begin(), type->modifiers.end());
                ACP_NODE(otherType);
              }
            }
          } else if (state.internalIndex == 2) {
            if (!exps.back()) ACP_NOT_OK;

            auto type = std::dynamic_pointer_cast<AST::Type>(ruleNode);

            type->returnType = std::dynamic_pointer_cast<AST::Type>(*exps.back().item);

            if (expect(TokenType::Pipe)) {
              state.internalIndex = 4;
              ACP_RULE(Type);
            }

            ACP_NODE(type);
          } else if (state.internalIndex == 4) {
            if (!exps.back()) ACP_NOT_OK;

            auto leftType = std::dynamic_pointer_cast<AST::Type>(ruleNode);
            auto rightType = std::dynamic_pointer_cast<AST::Type>(*exps.back().item);

            auto resultType = rightType->unionOf.size() > 0 ? rightType : nodeFactory.create<AST::Type>(std::vector<std::shared_ptr<AST::Type>> { rightType });
            resultType->unionOf.insert(resultType->unionOf.begin(), leftType);
            ACP_NODE(resultType);
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

            if (expect(TokenType::QuestionMark)) {
              auto wrapped = nodeFactory.create<AST::Type>();
              wrapped->lookup = type->lookup;
              wrapped->isNative = type->isNative;
              wrapped->name = type->name;
              type->lookup = nullptr;
              type->isNative = false;
              type->name = "";
              type->isOptional = true;
              type->optionalTarget = wrapped;
            }

            if (expect(TokenType::Pipe)) {
              state.internalIndex = 4;
              ACP_RULE(Type);
            }

            ACP_NODE(type);
          }
        } else if (rule == RuleType::IntegralLiteral) {
          auto integer = expect(TokenType::Integer);
          if (!integer) ACP_NOT_OK;
          ACP_NODE(nodeFactory.create<AST::IntegerLiteralNode>(integer.raw));
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
            ACP_NODE(nodeFactory.create<AST::ReturnDirectiveNode>(expr));
          }
        } else if (rule == RuleType::Block) {
          if (state.internalIndex == 0) {
            if (!expect(TokenType::OpeningBrace)) ACP_NOT_OK;

            state.internalIndex = 1;
            ruleNode = nodeFactory.create<AST::BlockNode>();

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
            auto varDef = nodeFactory.create<AST::VariableDefinitionExpression>();

            const auto saved = currentState;
            varDef->modifiers = expectModifiers(ModifierTargetType::Variable);

            if (!expectKeyword("let") && !expectKeyword("var")) {
              // root + module only statement + variable definition
              if (ruleStack.size() == 3) ACP_NOT_OK;
              // when the rule stack is greater than 3, this rule isn't being used on module level,
              // it's being used as an expression. so, we have to check for other expressions
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

            auto fetch = nodeFactory.create<AST::Fetch>(id.raw);

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
            ACP_RULE(Yield);
          } else if (state.internalIndex == 1) {
            if (!exps.back()) ACP_NOT_OK;

            auto assignment = nodeFactory.create<AST::AssignmentExpression>();
            assignment->target = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);

            ruleNode = std::move(assignment);

            state.internalIndex = 3;
            exps.clear();
            saveState();
            ACP_RULE(Attribute);
          } else if (state.internalIndex == 3) {
            if (exps.back()) {
              ACP_RULE(Attribute);
            }

            exps.pop_back();

            auto assignment = std::dynamic_pointer_cast<AST::AssignmentExpression>(ruleNode);
            for (auto& exp: exps) {
              assignment->attributes.push_back(std::dynamic_pointer_cast<AST::AttributeNode>(*exp.item));
            }

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
            } else if (expect(TokenType::BitwiseXorEquals)) {
              type = AT::BitwiseXor;
            } else {
              restoreState();
              ACP_NODE(assignment->target);
            }

            state.internalIndex = 2;
            assignment->type = type;

            ACP_RULE(Assignment);
          } else {
            if (!exps.back()) ACP_NOT_OK;

            auto assignment = std::dynamic_pointer_cast<AST::AssignmentExpression>(ruleNode);
            assignment->value = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);

            ACP_NODE(assignment);
          }
        } else if (rule == RuleType::AdditionOrSubtraction) {
          if (expectBinaryOperation(rule, RuleType::MultiplicationOrDivisionOrModulo, {
            { TokenType::PlusSign },
            { TokenType::MinusSign },
          }, {
            AST::OperatorType::Addition,
            AST::OperatorType::Subtraction,
          }, state, exps, ruleNode, next, saveState, restoreState)) {
            continue;
          }
        } else if (rule == RuleType::MultiplicationOrDivisionOrModulo) {
          if (expectBinaryOperation(rule, RuleType::Cast, {
            { TokenType::Asterisk },
            { TokenType::ForwardSlash },
            { TokenType::Percent },
          }, {
            AST::OperatorType::Multiplication,
            AST::OperatorType::Division,
            AST::OperatorType::Modulo,
          }, state, exps, ruleNode, next, saveState, restoreState)) {
            continue;
          }
        } else if (rule == RuleType::ModuleOnlyStatement) {
          if (state.iteration == 0) {
            while (expect(TokenType::Semicolon)); // optional
            ACP_RULE_LIST(
              RuleType::Import,
              RuleType::FunctionDefinition,
              RuleType::FunctionDeclaration,
              RuleType::ClassDefinition,
              RuleType::Structure,
              RuleType::TypeAlias,
              RuleType::VariableDeclaration,
              RuleType::Alias,
              RuleType::CodeLiteral,
              RuleType::Bitfield,
              RuleType::Enumeration,
              RuleType::VariableDefinition,
              RuleType::Export,

              // general attributes must come last because
              // they're supposed to be able to interpreted as part of
              // other statements that accept attributes if any such
              // statement is present
              RuleType::GeneralAttribute,
            );
          } else {
            while (expect(TokenType::Semicolon)); // optional
            if (!exps.back()) ACP_NOT_OK;
            auto item = *exps.back().item;
            if (auto var = std::dynamic_pointer_cast<AST::VariableDefinitionExpression>(item)) {
              auto stmt = nodeFactory.create<AST::ExpressionStatement>(var);
              ACP_NODE(stmt);
            } else if (auto expr = std::dynamic_pointer_cast<AST::ExpressionNode>(item)) {
              ACP_NOT_OK;
            }
            ACP_NODE(item);
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
            node = nodeFactory.create<AST::ImportStatement>(modName, alias);
          } else {
            node = nodeFactory.create<AST::ImportStatement>(modName, imports);
          }
          Timing::parseTimes[absoluteFilePath].stop();
          node->parse(filePath);
          Timing::parseTimes[absoluteFilePath].start();
          ACP_NODE(node);
        } else if (rule == RuleType::BooleanLiteral) {
          if (expectKeyword("true")) {
            ACP_NODE((nodeFactory.create<AST::BooleanLiteralNode>(true)));
          } else if (expectKeyword("false")) {
            ACP_NODE((nodeFactory.create<AST::BooleanLiteralNode>(false)));
          }
        } else if (rule == RuleType::FunctionCallOrSubscriptOrAccessorOrPostIncDec) {
          if (state.internalIndex == 0) {
            state.internalIndex = 6;
            ACP_RULE(Attribute);
          } else if (state.internalIndex == 6) {
            if (exps.back()) ACP_RULE(Attribute);
            exps.pop_back();
            state.internalIndex = 1;
            ACP_RULE(ClassInstantiation);
          } else if (state.internalIndex == 1) {
            if (!(exps.size() > 0 && exps.back()) && !ruleNode) ACP_NOT_OK;

            auto target = std::dynamic_pointer_cast<AST::ExpressionNode>(ruleNode ? ruleNode : *exps.back().item);
            if (!ruleNode) {
              exps.pop_back();
            }

            bool isCall = false;
            bool isSubscript = false;
            bool isAccessor = false;
            bool isPostIncrement = false;
            bool isPostDecrement = false;

            bool isMaybe = false;

            Token lastTok = currentState.lastToken;
            Token firstTok;
            Token maybeTok;

            saveState();

            if (maybeTok = expect(TokenType::QuestionMark)) {
              isMaybe = true;
            }

            if (firstTok = expect(TokenType::OpeningParenthesis)) {
              isCall = true;
            } else if (firstTok = expect(TokenType::OpeningSquareBracket)) {
              isSubscript = true;
            } else if (expect(TokenType::Dot) && expect(TokenType::Identifier)) {
              isAccessor = true;
            } else if (firstTok = expect(TokenType::Increment)) {
              isPostIncrement = true;
            } else if (firstTok = expect(TokenType::Decrement)) {
              isPostDecrement = true;
            } else {
              restoreState();
              for (auto exp: exps) {
                target->attributes.push_back(std::dynamic_pointer_cast<AST::AttributeNode>(*exp.item));
              }
              exps.clear();
              ACP_NODE(target);
            }

            // make sure the question mark is only used for function calls
            // AND
            // it is only considered an optional function call if there is no space
            // between the question mark and the parenthesis (e.g. `?(`)
            // ALSO
            // make sure that sensitive tokens like `[`, `(`, `++`, and `--`
            // only match when they're on the same line as the last token of the
            // child rule
            // this is so that this will be matched a function call:
            //     foo(bar)
            // and this will be too:
            //     foo(
            //       bar
            //     )
            // but not this:
            //     foo
            //     (bar)
            // or this:
            //     foo
            //     (
            //       bar
            //     )
            // same for the other sensitive tokens
            if (
              (
                !isCall &&
                isMaybe
              ) ||
              (
                isCall &&
                isMaybe &&
                (
                  firstTok.line != maybeTok.line ||
                  firstTok.column != maybeTok.column + 1
                )
              ) ||
              (
                firstTok &&
                firstTok.line != lastTok.line
              )
            ) {
              restoreState();
              for (auto exp: exps) {
                target->attributes.push_back(std::dynamic_pointer_cast<AST::AttributeNode>(*exp.item));
              }
              exps.clear();
              ACP_NODE(target);
            }

            if (isCall) {
              auto funcCall = nodeFactory.create<AST::FunctionCallExpression>();
              funcCall->target = target;
              funcCall->maybe = isMaybe;

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

              for (auto exp: exps) {
                funcCall->attributes.push_back(std::dynamic_pointer_cast<AST::AttributeNode>(*exp.item));
              }
              exps.clear();

              ruleNode = std::move(funcCall);
              state.internalIndex = 2;

              ACP_RULE(Expression);
            } else if (isSubscript) {
              auto subs = nodeFactory.create<AST::SubscriptExpression>();
              subs->target = target;

              for (auto exp: exps) {
                subs->attributes.push_back(std::dynamic_pointer_cast<AST::AttributeNode>(*exp.item));
              }
              exps.clear();

              ruleNode = std::move(subs);
              state.internalIndex = 3;

              ACP_RULE(Expression);
            } else if (isAccessor) {
              ruleNode = target;
              state.internalIndex = 5;

              restoreState(); // state 5 starts expecting dots and identifiers

              ACP_RULE(NullRule);
            } else {
              auto op = nodeFactory.create<AST::UnaryOperation>();
              op->target = target;

              if (isPostIncrement) {
                op->type = AST::UOperatorType::PostIncrement;
              } else {
                op->type = AST::UOperatorType::PostDecrement;
              }

              for (auto exp: exps) {
                op->attributes.push_back(std::dynamic_pointer_cast<AST::AttributeNode>(*exp.item));
              }
              exps.clear();

              ruleNode = std::move(op);

              ACP_RULE(NullRule);
            }
          } else if (state.internalIndex == 2) {
            auto callState = std::dynamic_pointer_cast<AST::FunctionCallExpression>(ruleNode);

            if (exps.back()) {
              callState->arguments.back().second = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);
              exps.pop_back();
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
              exps.pop_back();
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
            exps.pop_back();

            state.internalIndex = 1;
            ACP_RULE(NullRule);
          } else if (state.internalIndex == 4) {
            auto acc = std::dynamic_pointer_cast<AST::Accessor>(ruleNode);

            if (!acc) {
              restoreState();
              ACP_NODE(ruleNode);
            }

            if (!exps.back()) {
              exps.pop_back();
              if (acc->genericArguments.size() < 1) {
                restoreState();
                acc->genericArguments.clear();
                ACP_NODE(acc);
              }
            } else {
              acc->genericArguments.push_back(std::dynamic_pointer_cast<AST::Type>(*exps.back().item));
              exps.pop_back();

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
              ruleNode = nodeFactory.create<AST::Accessor>(target, id.raw);
              target = std::dynamic_pointer_cast<AST::ExpressionNode>(ruleNode);
              if (exps.size() > 0) {
                for (auto exp: exps) {
                  target->attributes.push_back(std::dynamic_pointer_cast<AST::AttributeNode>(*exp.item));
                }
                exps.clear();
              }
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
          ACP_NODE((nodeFactory.create<AST::StringLiteralNode>(Util::unescape(raw.raw.substr(1, raw.raw.length() - 2)))));
        } else if (rule == RuleType::Character) {
          auto raw = expect(TokenType::Character);
          if (!raw) ACP_NOT_OK;
          auto cont = raw.raw.substr(1, raw.raw.length() - 2);
          auto val = (cont.length() == 2) ? cont[1] : cont[0];
          auto escaped = cont.length() == 2;
          if (escaped) {
            switch (val) {
              case 'n': val = '\n'; break;
              case 'f': val = '\f'; break;
              case 'r': val = '\r'; break;
              case 't': val = '\t'; break;
              case 'v': val = '\v'; break;
              case '0': val = '\0'; break;
            }
          }
          ACP_NODE((nodeFactory.create<AST::CharacterLiteralNode>(val, escaped)));
        } else if (rule == RuleType::FunctionDeclaration) {
          if (state.internalIndex == 0) {
            state.internalIndex = 1;
            ACP_RULE(Attribute);
          } else if (state.internalIndex == 1) {
            if (exps.back()) ACP_RULE(Attribute);

            exps.pop_back();

            if (!expectKeyword("declare")) ACP_NOT_OK;

            auto funcDecl = nodeFactory.create<AST::FunctionDeclarationNode>();
            funcDecl->modifiers = expectModifiers(ModifierTargetType::Function);

            ruleNode = std::move(funcDecl);
            state.internalIndex = 2;
            ACP_RULE(Attribute);
          } else if (state.internalIndex == 2) {
            if (exps.back()) ACP_RULE(Attribute);

            exps.pop_back();

            auto funcDecl = std::dynamic_pointer_cast<AST::FunctionDeclarationNode>(ruleNode);
            auto tmpMods = expectModifiers(ModifierTargetType::Function);
            funcDecl->modifiers.insert(funcDecl->modifiers.end(), tmpMods.begin(), tmpMods.end());

            if (tmpMods.size() > 0) {
              ACP_RULE(Attribute);
            }

            for (auto& exp: exps) {
              funcDecl->attributes.push_back(std::dynamic_pointer_cast<AST::AttributeNode>(*exp.item));
            }
            exps.clear();

            if (!expectKeyword("function")) ACP_NOT_OK;

            auto name = expect(TokenType::Identifier);
            if (!name) ACP_NOT_OK;
            funcDecl->name = name.raw;

            if (!expect(TokenType::OpeningParenthesis)) ACP_NOT_OK;

            state.internalIndex = 3;
            ACP_RULE(Parameter);
          } else if (state.internalIndex == 3) {
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

            state.internalIndex = 4;
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

            auto attr = nodeFactory.create<AST::AttributeNode>();

            auto idExp = expect(TokenType::Identifier);
            size_t lastCol = 0;
            while (idExp) {
              attr->accessors.push_back(idExp.raw);
              lastCol = idExp.column + idExp.raw.length() - 1;
              if (!expect(TokenType::Dot)) break;
              idExp = expect(TokenType::Identifier);
            }
            if (attr->accessors.size() == 0) ACP_NOT_OK;

            saveState();

            auto paren = expect(TokenType::OpeningParenthesis);

            if (paren && paren.column == lastCol + 1) {
              ruleNode = std::move(attr);

              saveState();

              if (expectKeyword("type")) {
                state.internalIndex = 2;
                ACP_RULE(Type);
              } else {
                state.internalIndex = 1;
                ACP_RULE_LIST(
                  RuleType::IntegralLiteral,
                  RuleType::BooleanLiteral,
                  RuleType::String,
                  RuleType::Character,
                  RuleType::DecimalLiteral,
                  RuleType::StrictAccessor,
                  RuleType::Fetch
                );
              }
            } else {
              restoreState();
              ACP_NODE((std::move(attr)));
            }
          } else if (state.internalIndex == 2) {
            if (!exps.back()) {
              restoreState();
              state.internalIndex = 1;
              ACP_RULE_LIST(
                RuleType::IntegralLiteral,
                RuleType::BooleanLiteral,
                RuleType::String,
                RuleType::Character,
                RuleType::DecimalLiteral,
                RuleType::StrictAccessor,
                RuleType::Fetch
              );
            }

            auto attr = std::dynamic_pointer_cast<AST::AttributeNode>(ruleNode);
            attr->arguments.push_back(*exps.back().item);

            if (expect(TokenType::Comma)) {
              saveState();

              if (expectKeyword("type")) {
                state.internalIndex = 2;
                ACP_RULE(Type);
              } else {
                state.internalIndex = 1;
                ACP_RULE_LIST(
                  RuleType::IntegralLiteral,
                  RuleType::BooleanLiteral,
                  RuleType::String,
                  RuleType::Character,
                  RuleType::DecimalLiteral,
                  RuleType::StrictAccessor,
                  RuleType::Fetch
                );
              }
            }

            if (!expect(TokenType::ClosingParenthesis)) ACP_NOT_OK;

            ACP_NODE((attr));
          } else {
            auto attr = std::dynamic_pointer_cast<AST::AttributeNode>(ruleNode);

            if (exps.back()) {
              attr->arguments.push_back(*exps.back().item);

              if (expect(TokenType::Comma)) {
                saveState();

                if (expectKeyword("type")) {
                  state.internalIndex = 2;
                  ACP_RULE(Type);
                } else {
                  state.internalIndex = 1;
                  ACP_RULE_LIST(
                    RuleType::IntegralLiteral,
                    RuleType::BooleanLiteral,
                    RuleType::String,
                    RuleType::Character,
                    RuleType::DecimalLiteral,
                    RuleType::StrictAccessor,
                    RuleType::Fetch
                  );
                }
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
            ACP_NODE((nodeFactory.create<AST::AttributeStatement>(std::dynamic_pointer_cast<AST::AttributeNode>(*exps.back().item))));
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

            findingConditionalTest = true;
            ACP_RULE(Expression);
          } else if (state.internalIndex == 1) {
            findingConditionalTest = false;
            if (!exps.back()) ACP_NOT_OK;

            auto cond = nodeFactory.create<AST::ConditionalStatement>();
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
                findingConditionalTest = true;
                ACP_RULE(Expression);
              } else {
                state.internalIndex = 5;
                ACP_RULE(Statement);
              }
            }

            ACP_NODE(intern);
          } else if (state.internalIndex == 3) {
            findingConditionalTest = false;
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
                findingConditionalTest = true;
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
        } else if (rule == RuleType::PunctualConditonalExpression) {
          if (state.internalIndex == 0) {
            state.internalIndex = 1;
            ACP_RULE(Lambda);
          } else if (state.internalIndex == 1) {
            if (!exps.back()) ACP_NOT_OK;

            if (!expect(TokenType::QuestionMark)) ACP_EXP(exps.back().item);

            auto cond = nodeFactory.create<AST::ConditionalExpression>();
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
            { TokenType::OpeningAngleBracket },
            { TokenType::ClosingAngleBracket },
            { TokenType::LessThanOrEqualTo },
            { TokenType::GreaterThanOrEqualTo },
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
            { TokenType::Equality },
            { TokenType::Inequality },
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
            state.internalIndex = 4;
            ACP_RULE(Attribute);
          } else if (state.internalIndex == 4) {
            if (exps.back()) ACP_RULE(Attribute);

            // remove the last (always invalid) expectation
            exps.pop_back();

            auto mods = expectModifiers(ModifierTargetType::Class);
            auto def = nodeFactory.create<AST::ClassDefinitionNode>("");
            def->modifiers = mods;
            ruleNode = std::move(def);
            state.internalIndex = 5;
            ACP_RULE(Attribute);
          } else if (state.internalIndex == 5) {
            if (exps.back()) ACP_RULE(Attribute);

            // remove the last (always invalid) expectation
            exps.pop_back();

            auto klass = std::dynamic_pointer_cast<AST::ClassDefinitionNode>(ruleNode);
            auto mods = expectModifiers(ModifierTargetType::Class);
            klass->modifiers.insert(klass->modifiers.end(), mods.begin(), mods.end());

            for (auto& exp: exps) {
              klass->attributes.push_back(std::dynamic_pointer_cast<AST::AttributeNode>(*exp.item));
            }

            exps.clear();

            if (!expectKeyword("class")) ACP_NOT_OK;

            auto id = expect(TokenType::Identifier);
            if (!id) ACP_NOT_OK;
            klass->name = id.raw;

            if (expect(TokenType::OpeningAngleBracket)) {
              state.internalIndex = 2;
              ACP_RULE(Generic);
            }

            if (expectKeyword("extends")) {
              state.internalIndex = 3;
              ACP_RULE(StrictAccessor);
            }

            if (!expect(TokenType::OpeningBrace)) ACP_NOT_OK;

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
          } else if (state.internalIndex == 3) {
            if (!exps.back()) ACP_NOT_OK;

            auto klass = std::dynamic_pointer_cast<AST::ClassDefinitionNode>(ruleNode);

            klass->parents.push_back(std::dynamic_pointer_cast<AST::RetrievalNode>(*exps.back().item));

            if (expect(TokenType::Comma)) {
              ACP_RULE(StrictAccessor);
            }

            if (!expect(TokenType::OpeningBrace)) ACP_NOT_OK;

            inClass = true;

            state.internalIndex = 1;
            ACP_RULE(ClassStatement);
          } else if (state.internalIndex == 1) {
            auto klass = std::dynamic_pointer_cast<AST::ClassDefinitionNode>(ruleNode);

            if (exps.back()) {
              klass->statements.push_back(std::dynamic_pointer_cast<AST::ClassStatementNode>(*exps.back().item));
              ACP_RULE(ClassStatement);
            }

            inClass = false;

            if (!expect(TokenType::ClosingBrace)) ACP_NOT_OK;

            ACP_NODE((klass));
          }
        } else if (rule == RuleType::ClassStatement) {
          if (state.iteration == 0) {
            ACP_RULE_LIST(
              RuleType::ClassMember,
              RuleType::ClassSpecialMethod,
              RuleType::ClassMethod,
              RuleType::OperatorDefinition
            );
          }

          if (!exps.back()) ACP_NOT_OK;

          while (expect(TokenType::Semicolon)); // optional

          ACP_EXP(exps.back().item);
        } else if (rule == RuleType::ClassMember) {
          if (state.internalIndex == 0) {
            auto visibilityMod = expectModifier(ModifierTargetType::ClassStatement);
            if (!visibilityMod) ACP_NOT_OK;

            ruleNode = nodeFactory.create<AST::ClassMemberDefinitionStatement>(AST::parseVisibility(*visibilityMod));
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
            ruleNode = nodeFactory.create<AST::ClassMethodDefinitionStatement>(AST::parseVisibility(*visibilityMod));
            state.internalIndex = 2;
            ACP_RULE(FunctionDefinition);
          } else {
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

            auto method = nodeFactory.create<AST::ClassSpecialMethodDefinitionStatement>(AST::parseVisibility(*visibilityMod), AST::SpecialClassMethod::Constructor);

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

            bool methodLike = true;

            auto kind = AST::SpecialClassMethod::Constructor;
            if (expectKeyword("constructor")) {
              kind = AST::SpecialClassMethod::Constructor;
            } else if (expectKeyword("destructor")) {
              kind = AST::SpecialClassMethod::Destructor;
            } else if (expectKeyword("from")) {
              methodLike = false;
              kind = AST::SpecialClassMethod::From;
            } else if (expectKeyword("to")) {
              methodLike = false;
              kind = AST::SpecialClassMethod::To;
            } else {
              ACP_NOT_OK;
            }

            method->type = kind;

            if (methodLike) {
              if (!expect(TokenType::OpeningParenthesis)) ACP_NOT_OK;

              state.internalIndex = 3;
              ACP_RULE(Parameter);
            } else {
              state.internalIndex = 5;
              ACP_RULE(Type);
            }
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
            ACP_RULE(Block);
          } else if (state.internalIndex == 4) {
            if (!exps.back()) ACP_NOT_OK;

            auto method = std::dynamic_pointer_cast<AST::ClassSpecialMethodDefinitionStatement>(ruleNode);
            method->body = std::dynamic_pointer_cast<AST::BlockNode>(*exps.back().item);

            ACP_NODE((method));
          } else if (state.internalIndex == 5) {
            if (!exps.back()) ACP_NOT_OK;

            auto method = std::dynamic_pointer_cast<AST::ClassSpecialMethodDefinitionStatement>(ruleNode);
            method->specialType = std::dynamic_pointer_cast<AST::Type>(*exps.back().item);

            state.internalIndex = 4;
            ACP_RULE(Block);
          }
        } else if (rule == RuleType::ClassInstantiation) {
          if (state.internalIndex == 0) {
            saveState();
            if (!expectKeyword("new")) {
              state.internalIndex = 4;
              if (inClass) {
                ruleNode = nodeFactory.create<AST::ClassInstantiationExpression>();
                state.internalIndex = 5;
                ACP_RULE(SuperClassFetch);
              }
              ACP_RULE_LIST(
                RuleType::Sizeof,
                RuleType::Nullptr,
                RuleType::BooleanLiteral,
                RuleType::IntegralLiteral,
                RuleType::String,
                RuleType::Character,
                RuleType::Accessor,
                RuleType::DecimalLiteral
              );
            }

            auto inst = nodeFactory.create<AST::ClassInstantiationExpression>();

            if (expectKeyword("persistent") || expect(TokenType::Asterisk)) {
              inst->persistent = true;
            }

            ruleNode = std::move(inst);
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
                RuleType::Nullptr,
                RuleType::BooleanLiteral,
                RuleType::IntegralLiteral,
                RuleType::String,
                RuleType::Character,
                RuleType::Accessor,
                RuleType::DecimalLiteral
              );
            }

            auto inst = std::dynamic_pointer_cast<AST::ClassInstantiationExpression>(ruleNode);

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
              ACP_RULE(Expression);
            } else if (isSuperclassFetch) {
              restoreState();
              state.internalIndex = 4;
              ACP_RULE_LIST(
                RuleType::Sizeof,
                RuleType::Nullptr,
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
            ACP_RULE(Await);
          } else if (state.internalIndex == 1) {
            if (!exps.back()) ACP_NOT_OK;
            auto cast = nodeFactory.create<AST::CastExpression>();
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
                exprs.push(nodeFactory.create<AST::PointerExpression>());
              } else if (isDereference) {
                exprs.push(nodeFactory.create<AST::DereferenceExpression>());
              } else {
                auto expr = nodeFactory.create<AST::UnaryOperation>();
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
            findingConditionalTest = true;
            ACP_RULE(Expression);
          } else if (state.internalIndex == 1) {
            findingConditionalTest = false;
            if (!exps.back()) ACP_NOT_OK;

            auto loop = nodeFactory.create<AST::WhileLoopStatement>();
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
            state.internalIndex = 1;
            ACP_RULE(Attribute);
          } else if (state.internalIndex == 1) {
            if (exps.back()) ACP_RULE(Attribute);

            exps.pop_back();

            auto typeAlias = ruleNode ? std::dynamic_pointer_cast<AST::TypeAliasStatement>(ruleNode) : nodeFactory.create<AST::TypeAliasStatement>();
            auto mods = expectModifiers(ModifierTargetType::TypeAlias);
            typeAlias->modifiers.insert(typeAlias->modifiers.end(), mods.begin(), mods.end());

            ruleNode = std::move(typeAlias);
            state.internalIndex = 2;
            ACP_RULE(Attribute);
          } else if (state.internalIndex == 2) {
            if (exps.back()) {
              state.internalIndex = 1;
              ACP_RULE(Attribute);
            }

            exps.pop_back();

            auto typeAlias = std::dynamic_pointer_cast<AST::TypeAliasStatement>(ruleNode);

            for (auto& exp: exps) {
              typeAlias->attributes.push_back(std::dynamic_pointer_cast<AST::AttributeNode>(*exp.item));
            }
            exps.clear();

            if (!expectKeyword("type")) ACP_NOT_OK;

            auto name = expect(TokenType::Identifier);
            if (!name) ACP_NOT_OK;

            if (!expect(TokenType::EqualSign)) ACP_NOT_OK;

            typeAlias->name = name.raw;

            if (expectKeyword("any")) {
              typeAlias->type = nodeFactory.create<AST::Type>();
              typeAlias->type->isAny = true;
              ACP_NODE(typeAlias);
            }

            state.internalIndex = 3;
            ACP_RULE(Type);
          } else if (state.internalIndex == 3) {
            if (!exps.back()) ACP_NOT_OK;
            auto typeAlias = std::dynamic_pointer_cast<AST::TypeAliasStatement>(ruleNode);
            typeAlias->type = std::dynamic_pointer_cast<AST::Type>(*exps.back().item);
            ACP_NODE(typeAlias);
          }
        } else if (rule == RuleType::SuperClassFetch) {
          if (state.internalIndex == 0) {
            if (!expectKeyword("super")) ACP_NOT_OK;
            auto sup = nodeFactory.create<AST::SuperClassFetch>();
            if (expect(TokenType::OpeningAngleBracket)) {
              if (auto lit = expect(TokenType::Integer)) {
                sup->fetch = nodeFactory.create<AST::IntegerLiteralNode>(lit.raw);
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
            auto instOf = nodeFactory.create<AST::InstanceofExpression>();
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
            auto node = nodeFactory.create<AST::Generic>(name.raw);
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

            ruleNode = nodeFactory.create<AST::ForLoopStatement>();
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

            auto loop = nodeFactory.create<AST::RangedForLoopStatement>();
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
              state.internalIndex = 4;
              ACP_RULE(Statement);
            }

            state.internalIndex = 3;
            findingConditionalTest = true;
            ACP_RULE(Expression);
          } else if (state.internalIndex == 3) {
            findingConditionalTest = false;
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
                RuleType::StrictAccessor,
                RuleType::Fetch,
                RuleType::SpecialFetch,
                RuleType::GroupedExpression,
              );
            } else {
              ACP_RULE_LIST(
                RuleType::StrictAccessor,
                RuleType::Fetch,
                RuleType::SpecialFetch,
                RuleType::GroupedExpression,
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
            auto op = nodeFactory.create<AST::SizeofOperation>();
            op->target = std::dynamic_pointer_cast<AST::Type>(*exps.back().item);
            ACP_NODE(op);
          }
        } else if (rule == RuleType::And) {
          if (expectBinaryOperation(RuleType::And, RuleType::BitwiseOr, {
            { TokenType::And }
          }, {
            AST::OperatorType::LogicalAnd,
          }, state, exps, ruleNode, next, saveState, restoreState)) {
            continue;
          }
        } else if (rule == RuleType::Or) {
          if (expectBinaryOperation(RuleType::Or, RuleType::And, {
            { TokenType::Or }
          }, {
            AST::OperatorType::LogicalOr,
          }, state, exps, ruleNode, next, saveState, restoreState)) {
            continue;
          }
        } else if (rule == RuleType::Shift) {
          if (expectBinaryOperation(RuleType::Shift, RuleType::AdditionOrSubtraction, {
            { TokenType::LeftShift },
            { TokenType::ClosingAngleBracket, TokenType::ClosingAngleBracket },
          }, {
            AST::OperatorType::LeftShift,
            AST::OperatorType::RightShift,
          }, state, exps, ruleNode, next, saveState, restoreState)) {
            continue;
          }
        } else if (rule == RuleType::BitwiseAnd) {
          if (expectBinaryOperation(RuleType::BitwiseAnd, RuleType::EqualityRelationalOperation, {
            { TokenType::Ampersand },
          }, {
            AST::OperatorType::BitwiseAnd,
          }, state, exps, ruleNode, next, saveState, restoreState)) {
            continue;
          }
        } else if (rule == RuleType::BitwiseOr) {
          if (expectBinaryOperation(RuleType::BitwiseOr, RuleType::BitwiseXor, {
            { TokenType::Pipe },
          }, {
            AST::OperatorType::BitwiseOr,
          }, state, exps, ruleNode, next, saveState, restoreState)) {
            continue;
          }
        } else if (rule == RuleType::BitwiseXor) {
          if (expectBinaryOperation(RuleType::BitwiseXor, RuleType::BitwiseAnd, {
            { TokenType::Caret },
          }, {
            AST::OperatorType::BitwiseXor,
          }, state, exps, ruleNode, next, saveState, restoreState)) {
            continue;
          }
        } else if (rule == RuleType::DecimalLiteral) {
          auto decimal = expect(TokenType::Decimal);
          if (!decimal) ACP_NOT_OK;
          ACP_NODE(nodeFactory.create<AST::FloatingPointLiteralNode>(decimal.raw));
        } else if (rule == RuleType::Structure) {
          if (state.internalIndex == 0) {
            state.internalIndex = 1;
            ACP_RULE(Attribute);
          } else if (state.internalIndex == 1) {
            if (exps.back()) ACP_RULE(Attribute);

            exps.pop_back();

            auto structure = ruleNode ? std::dynamic_pointer_cast<AST::StructureDefinitionStatement>(ruleNode) : nodeFactory.create<AST::StructureDefinitionStatement>();
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

            auto statement = nodeFactory.create<AST::ExportStatement>();
            ruleNode = statement;

            if (expect(TokenType::Asterisk)) {
              statement->externalTarget = nodeFactory.create<AST::ImportStatement>();
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
              state.internalIndex = 3;
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
              statement->externalTarget = nodeFactory.create<AST::ImportStatement>();
              state.internalIndex = 2;
              ACP_RULE(NullRule);
            }

            Token alias;
            if (expectKeyword("as")) {
              alias = expect(TokenType::Identifier);
            }

            if (expectKeyword("from") && expect(TokenType::String)) {
              restoreState();
              statement->externalTarget = nodeFactory.create<AST::ImportStatement>();
              state.internalIndex = 2;
              ACP_RULE(NullRule);
            }

            statement->localTargets.push_back(std::make_pair(std::dynamic_pointer_cast<AST::RetrievalNode>(*exps.back().item), alias ? alias.raw : ""));

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
          } else if (state.internalIndex == 3 || state.internalIndex == 4) {
            auto statement = std::dynamic_pointer_cast<AST::ExportStatement>(ruleNode);

            if (state.internalIndex == 3) {
              state.internalIndex = 4;
              ACP_RULE(StrictAccessor);
            } else if (exps.back()) {
              statement->localTargets.push_back(std::make_pair(std::dynamic_pointer_cast<AST::RetrievalNode>(*exps.back().item), ""));
              if (expectKeyword("as")) {
                auto alias = expect(TokenType::Identifier);
                if (!alias) {
                  ACP_NOT_OK;
                } else {
                  statement->localTargets.back().second = alias.raw;
                }
              }
              if (expect(TokenType::Comma)) {
                state.internalIndex = 3;
                ACP_RULE(NullRule);
              }
            }

            if (!expect(TokenType::ClosingBrace)) ACP_NOT_OK;

            if (expectKeyword("from")) {
              statement->localTargets.clear();
              restoreState();
              statement->externalTarget = nodeFactory.create<AST::ImportStatement>();
              state.internalIndex = 2;
              state.internalValue = true;
              ACP_RULE(NullRule);
            }

            ACP_NODE(statement);
          }
        } else if (rule == RuleType::VariableDeclaration) {
          if (state.internalIndex == 0) {
            if (!expectKeyword("declare")) ACP_NOT_OK;

            state.internalIndex = 1;
            ACP_RULE(Attribute);
          } else if (state.internalIndex == 1) {
            if (exps.back()) ACP_RULE(Attribute);

            exps.pop_back();

            auto var = ruleNode ? std::dynamic_pointer_cast<AST::VariableDeclarationStatement>(ruleNode) : nodeFactory.create<AST::VariableDeclarationStatement>();
            auto mods = expectModifiers(ModifierTargetType::Variable);
            var->modifiers.insert(var->modifiers.end(), mods.begin(), mods.end());

            ruleNode = std::move(var);
            state.internalIndex = 2;
            ACP_RULE(Attribute);
          } else if (state.internalIndex == 2) {
            if (exps.back()) {
              state.internalIndex = 1;
              ACP_RULE(Attribute);
            }

            exps.pop_back();

            auto var = std::dynamic_pointer_cast<AST::VariableDeclarationStatement>(ruleNode);

            for (auto& exp: exps) {
              var->attributes.push_back(std::dynamic_pointer_cast<AST::AttributeNode>(*exp.item));
            }
            exps.clear();

            if (!expectKeyword("let") && !expectKeyword("var")) ACP_NOT_OK;

            auto id = expect(TokenType::Identifier);
            if (!id) ACP_NOT_OK;

            var->name = id.raw;

            if (!expect(TokenType::Colon)) ACP_NOT_OK;

            state.internalIndex = 3;
            ACP_RULE(Type);
          } else if (state.internalIndex == 3) {
            if (!exps.back()) ACP_NOT_OK;

            auto var = std::dynamic_pointer_cast<AST::VariableDeclarationStatement>(ruleNode);

            var->type = std::dynamic_pointer_cast<AST::Type>(*exps.back().item);

            ACP_NODE(var);
          }
        } else if (rule == RuleType::Alias) {
          if (state.internalIndex == 0) {
            if (!expectKeyword("using")) ACP_NOT_OK;

            auto name = expect(TokenType::Identifier);
            if (!name) ACP_NOT_OK;

            if (!expect(TokenType::EqualSign)) ACP_NOT_OK;

            auto alias = nodeFactory.create<AST::AliasStatement>();
            alias->name = name.raw;

            ruleNode = std::move(alias);
            state.internalIndex = 1;
            ACP_RULE(StrictAccessor);
          } else {
            if (!exps.back()) ACP_NOT_OK;

            auto alias = std::dynamic_pointer_cast<AST::AliasStatement>(ruleNode);
            alias->target = std::dynamic_pointer_cast<AST::RetrievalNode>(*exps.back().item);

            ACP_NODE(alias);
          }
        } else if (rule == RuleType::Delete) {
          if (state.internalIndex == 0) {
            if (!expectKeyword("delete")) ACP_NOT_OK;

            auto del = nodeFactory.create<AST::DeleteStatement>();

            if (expectKeyword("persistent") || expect(TokenType::Asterisk)) {
              del->persistent = true;
            }

            ruleNode = std::move(del);
            state.internalIndex = 1;

            ACP_RULE(Expression);
          } else {
            if (!exps.back()) ACP_NOT_OK;

            auto del = std::dynamic_pointer_cast<AST::DeleteStatement>(ruleNode);
            del->target = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);

            ACP_NODE(del);
          }
        } else if (rule == RuleType::ControlDirective) {
          auto ctrl = nodeFactory.create<AST::ControlDirective>();
          if (expectKeyword("continue")) {
            ctrl->isBreak = false;
          } else if (expectKeyword("break")) {
            ctrl->isBreak = true;
          } else {
            ACP_NOT_OK;
          }
          ACP_NODE(ctrl);
        } else if (rule == RuleType::TryCatch) {
          if (state.internalIndex == 0) {
            if (!expectKeyword("try")) ACP_NOT_OK;
            state.internalIndex = 1;
            ACP_RULE(Statement);
          } else if (state.internalIndex == 1) {
            if (!exps.back()) ACP_NOT_OK;
            auto blk = nodeFactory.create<AST::TryCatchBlock>();
            ruleNode = blk;
            blk->tryBlock = std::dynamic_pointer_cast<AST::StatementNode>(*exps.back().item);
            if (!expectKeyword("catch")) ACP_NOT_OK;
            state.internalIndex = 2;
            ACP_RULE(NullRule);
          } else if (state.internalIndex == 2) {
            if (expectKeyword("all")) {
              state.internalIndex = 3;
              ACP_RULE(Statement);
            }
            auto id = expect(TokenType::Identifier);
            if (!id) ACP_NOT_OK;
            if (!expect(TokenType::Colon)) ACP_NOT_OK;
            auto blk = std::dynamic_pointer_cast<AST::TryCatchBlock>(ruleNode);
            blk->catchIDs.push_back(id.raw);
            state.internalIndex = 5;
            ACP_RULE(Type);
          } else if (state.internalIndex == 3 || state.internalIndex == 4) {
            if (!exps.back()) ACP_NOT_OK;
            auto blk = std::dynamic_pointer_cast<AST::TryCatchBlock>(ruleNode);
            if (state.internalIndex == 3) {
              blk->catchAllBlock = std::dynamic_pointer_cast<AST::StatementNode>(*exps.back().item);
            } else {
              blk->catchBlocks.back().second = std::dynamic_pointer_cast<AST::StatementNode>(*exps.back().item);
            }
            if (expectKeyword("catch")) {
              state.internalIndex = 2;
              ACP_RULE(NullRule);
            }
            ACP_NODE(blk);
          } else if (state.internalIndex == 5) {
            if (!exps.back()) ACP_NOT_OK;
            auto blk = std::dynamic_pointer_cast<AST::TryCatchBlock>(ruleNode);
            blk->catchBlocks.push_back(std::make_pair(std::dynamic_pointer_cast<AST::Type>(*exps.back().item), std::shared_ptr<AST::StatementNode>(nullptr)));
            state.internalIndex = 4;
            ACP_RULE(Statement);
          }
        } else if (rule == RuleType::Throw) {
          if (state.internalIndex == 0) {
            if (!expectKeyword("throw")) ACP_NOT_OK;
            state.internalIndex = 1;
            ACP_RULE(Expression);
          } else if (state.internalIndex == 1) {
            if (!exps.back()) ACP_NOT_OK;
            auto stmt = nodeFactory.create<AST::ThrowStatement>();
            stmt->expression = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);
            ACP_NODE(stmt);
          }
        } else if (rule == RuleType::Nullptr) {
          if (!expectKeyword("nullptr") && !expectKeyword("null")) ACP_NOT_OK;
          auto node = nodeFactory.create<AST::NullptrExpression>();
          ACP_NODE(node);
        } else if (rule == RuleType::CodeLiteral) {
          if (state.iteration == 0) {
            ACP_RULE(Attribute);
          } else {
            if (exps.back()) ACP_RULE(Attribute);
            exps.pop_back();

            auto lit = expect(TokenType::Code);
            if (!lit) ACP_NOT_OK;

            auto node = nodeFactory.create<AST::CodeLiteralNode>(lit.raw.substr(3, lit.raw.size() - 6));

            for (auto& exp: exps) {
              node->attributes.push_back(std::dynamic_pointer_cast<AST::AttributeNode>(*exp.item));
            }
            exps.clear();

            ACP_NODE(node);
          }
        } else if (rule == RuleType::Bitfield) {
          if (state.internalIndex == 0) {
            state.internalIndex = 1;
            ACP_RULE(Attribute);
          } else if (state.internalIndex == 1) {
            if (exps.back()) ACP_RULE(Attribute);

            exps.pop_back();

            auto bits = nodeFactory.create<AST::BitfieldDefinitionNode>();
            bits->modifiers = expectModifiers(ModifierTargetType::Bitfield);

            ruleNode = std::move(bits);
            state.internalIndex = 2;
            ACP_RULE(Attribute);
          } else if (state.internalIndex == 2) {
            if (exps.back()) ACP_RULE(Attribute);

            exps.pop_back();

            auto bits = std::dynamic_pointer_cast<AST::BitfieldDefinitionNode>(ruleNode);

            for (auto& exp: exps) {
              bits->attributes.push_back(std::dynamic_pointer_cast<AST::AttributeNode>(*exp.item));
            }
            exps.clear();

            if (!expectKeyword("bitfield")) ACP_NOT_OK;

            auto name = expect(TokenType::Identifier);
            if (!name) ACP_NOT_OK;
            bits->name = name.raw;

            if (expect(TokenType::Colon)) {
              state.internalIndex = 3;
              ACP_RULE(Type);
            } else {
              state.internalIndex = 4;
              ACP_RULE(NullRule);
            }
          } else if (state.internalIndex == 3 || state.internalIndex == 4) {
            auto bits = std::dynamic_pointer_cast<AST::BitfieldDefinitionNode>(ruleNode);

            if (state.internalIndex == 3) {
              if (!exps.back()) ACP_NOT_OK;
              bits->underlyingType = std::dynamic_pointer_cast<AST::Type>(*exps.back().item);
            }

            if (!expect(TokenType::OpeningBrace)) ACP_NOT_OK;
            if (expect(TokenType::ClosingBrace)) ACP_NODE(bits);

            state.internalIndex = 5;
            ACP_RULE(NullRule);
          } else {
            auto bits = std::dynamic_pointer_cast<AST::BitfieldDefinitionNode>(ruleNode);

            if (state.internalIndex == 5) {
              auto field = expect(TokenType::Identifier);
              if (!field) ACP_NOT_OK;
              bits->members.push_back(std::make_tuple(nullptr, field.raw, 0, 0));

              if (expect(TokenType::Colon)) {
                state.internalIndex = 6;
                ACP_RULE(Type);
              } else {
                state.internalIndex = 7;
                ACP_RULE(NullRule);
              }
            } else if (state.internalIndex == 6) {
              if (!exps.back()) ACP_NOT_OK;
              std::get<0>(bits->members.back()) = std::dynamic_pointer_cast<AST::Type>(*exps.back().item);
              state.internalIndex = 7;
              ACP_RULE(NullRule);
            } else {
              auto& [type, name, start, end] = bits->members.back();

              if (!expect(TokenType::EqualSign)) ACP_NOT_OK;
              auto startStr = expect(TokenType::Integer);
              if (!startStr) ACP_NOT_OK;
              start = end = AST::IntegerLiteralNode::parseInteger(startStr.raw);
              if (expect(TokenType::Dot)) {
                if (!expect(TokenType::Dot)) ACP_NOT_OK;
                bool inclusive = (bool)expect(TokenType::Dot);
                auto endStr = expect(TokenType::Integer);
                if (!endStr) ACP_NOT_OK;
                end = AST::IntegerLiteralNode::parseInteger(endStr.raw) - (inclusive ? 0 : 1);
              }
              // optional semicolons
              while (expect(TokenType::Semicolon));

              if (expect(TokenType::ClosingBrace)) {
                ACP_NODE(bits);
              } else {
                state.internalIndex = 5;
                ACP_RULE(NullRule);
              }
            }
          }
        } else if (rule == RuleType::Lambda) {
          #define LAMBDA_RESTORE {  restoreState(); state.internalIndex = 1; ACP_RULE(Or); }
          if (state.internalIndex == 0) {
            saveState();

            auto lambda = nodeFactory.create<AST::LambdaExpression>();
            lambda->modifiers = expectModifiers(ModifierTargetType::Lambda);

            ruleNode = std::move(lambda);

            state.internalIndex = 8;
            ACP_RULE(Attribute);
          } else if (state.internalIndex == 1) {
            ACP_EXP(exps.back().item);
          } else if (state.internalIndex == 2) {
            auto lambda = std::dynamic_pointer_cast<AST::LambdaExpression>(ruleNode);

            lambda->isGenerator = std::find(lambda->modifiers.begin(), lambda->modifiers.end(), "generator") != lambda->modifiers.end();
            lambda->isAsync = std::find(lambda->modifiers.begin(), lambda->modifiers.end(), "async") != lambda->modifiers.end();

            if (!expect(TokenType::OpeningParenthesis)) LAMBDA_RESTORE;

            state.internalIndex = expect(TokenType::ClosingParenthesis) ? 5 : 3;
            ACP_RULE(NullRule);
          } else if (state.internalIndex == 3) {
            auto lambda = std::dynamic_pointer_cast<AST::LambdaExpression>(ruleNode);

            if (lambda->parameters.size() > 0) {
              if (!expect(TokenType::Comma)) {
                if (!expect(TokenType::ClosingParenthesis)) LAMBDA_RESTORE;
                state.internalIndex = 5;
                ACP_RULE(NullRule);
              }
            }

            auto name = expect(TokenType::Identifier);
            if (!name) LAMBDA_RESTORE;

            if (!expect(TokenType::Colon)) LAMBDA_RESTORE;

            auto param = nodeFactory.create<AST::Parameter>();
            param->name = name.raw;
            lambda->parameters.push_back(param);

            state.internalIndex = 4;
            ACP_RULE(Type);
          } else if (state.internalIndex == 4) {
            if (!exps.back()) LAMBDA_RESTORE;

            auto lambda = std::dynamic_pointer_cast<AST::LambdaExpression>(ruleNode);

            lambda->parameters.back()->type = std::dynamic_pointer_cast<AST::Type>(*exps.back().item);

            state.internalIndex = 3;
            ACP_RULE(NullRule);
          } else if (state.internalIndex == 5) {
            if (!expect(TokenType::FatReturns)) LAMBDA_RESTORE;

            state.internalIndex = 6;
            ACP_RULE(Type);
          } else if (state.internalIndex == 6) {
            if (!exps.back()) LAMBDA_RESTORE;

            auto lambda = std::dynamic_pointer_cast<AST::LambdaExpression>(ruleNode);

            lambda->returnType = std::dynamic_pointer_cast<AST::Type>(*exps.back().item);

            state.internalIndex = 7;
            ACP_RULE(Block);
          } else if (state.internalIndex == 7) {
            if (!exps.back()) LAMBDA_RESTORE;

            auto lambda = std::dynamic_pointer_cast<AST::LambdaExpression>(ruleNode);

            lambda->body = std::dynamic_pointer_cast<AST::BlockNode>(*exps.back().item);

            ACP_NODE(lambda);
          } else if (state.internalIndex == 8) {
            if (exps.back()) ACP_RULE(Attribute);

            auto lambda = std::dynamic_pointer_cast<AST::LambdaExpression>(ruleNode);
            auto tmp = expectModifiers(ModifierTargetType::Lambda);
            lambda->modifiers.insert(lambda->modifiers.end(), tmp.begin(), tmp.end());

            exps.pop_back();

            if (tmp.size() > 0) {
              ACP_RULE(Attribute);
            }

            for (auto& exp: exps) {
              lambda->attributes.push_back(std::dynamic_pointer_cast<AST::AttributeNode>(*exp.item));
            }
            exps.clear();

            state.internalIndex = 2;
            ACP_RULE(NullRule);
          }
          #undef LAMBDA_RESTORE
        } else if (rule == RuleType::SpecialFetch) {
          if (state.internalIndex == 0) {
            state.internalIndex = 1;
            ACP_RULE(Attribute);
          } else {
            if (exps.back()) ACP_RULE(Attribute);

            exps.pop_back();

            auto id = expect(TokenType::SpecialIdentifier);
            if (!id) ACP_NOT_OK;

            auto special = nodeFactory.create<AST::SpecialFetchExpression>();
            special->query = id.raw;

            for (auto& exp: exps) {
              special->attributes.push_back(std::dynamic_pointer_cast<AST::AttributeNode>(*exp.item));
            }

            ACP_NODE(special);
          }
        } else if (rule == RuleType::OperatorDefinition) {
          if (state.internalIndex == 0) {
            state.internalIndex = 1;
            ACP_RULE(Attribute);
          } else if (state.internalIndex == 1) {
            if (exps.back()) ACP_RULE(Attribute);

            exps.pop_back();

            auto visibilityMod = expectModifier(ModifierTargetType::ClassStatement);
            if (!visibilityMod) ACP_NOT_OK;

            ruleNode = nodeFactory.create<AST::ClassOperatorDefinitionStatement>(AST::parseVisibility(*visibilityMod));

            // optional, but make sure we expect a closing parenthesis later
            state.internalValue = !!expect(TokenType::OpeningParenthesis);

            state.internalIndex = 2;
            ACP_RULE(NullRule);
          } else if (state.internalIndex == 2) {
            using COT = AST::ClassOperatorType;
            using COO = AST::ClassOperatorOrientation;
            COT type = COT::NONE;
            COO orient = COO::Unary;
            auto op = std::dynamic_pointer_cast<AST::ClassOperatorDefinitionStatement>(ruleNode);
            if (expect(TokenType::ExclamationMark)) {
              type = COT::Not;
            } else if (expect(TokenType::Asterisk)) {
              type = COT::Dereference;
            } else if (expect(TokenType::Ampersand)) {
              type = COT::Reference;
            } else if (expect(TokenType::Tilde)) {
              type = COT::BitNot;
            } else if (expectKeyword("this")) {
              if (expect(TokenType::OpeningSquareBracket)) {
                state.internalIndex = 10;
                op->orientation = COO::Unary;
                op->type = COT::Index;
                ACP_RULE(Type);
              } else {
                op->orientation = COO::Left;
                state.internalIndex = 6;
                ACP_RULE(NullRule);
              }
            } else {
              op->orientation = COO::Right;
              state.internalIndex = 8;
              ACP_RULE(Type);
            }

            op->type = type;
            op->orientation = orient;

            // if we got here, it means it's an unary operator and we need `this`
            if (!expectKeyword("this")) ACP_NOT_OK;
            state.internalIndex = 3;
            ACP_RULE(NullRule);
          } else if (state.internalIndex == 3) {
            if (ALTACORE_ANY_CAST<bool>(state.internalValue)) {
              if (!expect(TokenType::ClosingParenthesis)) ACP_NOT_OK;
            }

            if (!expect(TokenType::Colon)) ACP_NOT_OK;
            state.internalIndex = 4;
            ACP_RULE(Type);
          } else if (state.internalIndex == 4) {
            if (!exps.back()) ACP_NOT_OK;

            auto op = std::dynamic_pointer_cast<AST::ClassOperatorDefinitionStatement>(ruleNode);

            op->returnType = std::dynamic_pointer_cast<AST::Type>(*exps.back().item);

            state.internalIndex = 5;
            ACP_RULE(Block);
          } else if (state.internalIndex == 5) {
            if (!exps.back()) ACP_NOT_OK;

            auto op = std::dynamic_pointer_cast<AST::ClassOperatorDefinitionStatement>(ruleNode);

            op->block = std::dynamic_pointer_cast<AST::BlockNode>(*exps.back().item);

            ACP_NODE(op);
          } else if (state.internalIndex == 6 || state.internalIndex == 7) {
            using COT = AST::ClassOperatorType;
            COT type = COT::NONE;
            saveState();
            bool leftThis = state.internalIndex == 6;

            // AC_AS_OP_SYM = AltaCore::Assignment::Operator::Symbol
            #define AC_AS_OP_SYM(tok, name) else if (leftThis && expect(TokenType::tok##Equals)) { type = COT::name##Assignment; }
            // AC_AS_OP_DSYM = AltaCore::Assignment::Operator::DirectSymbol
            #define AC_AS_OP_DSYM(name) AC_AS_OP_SYM(name, name)

            if (expect(TokenType::PlusSign)) {
              type = COT::Addition;
            } else if (expect(TokenType::MinusSign)) {
              type = COT::Subtraction;
            } else if (expect(TokenType::Asterisk)) {
              type = COT::Multiplication;
            } else if (expect(TokenType::ForwardSlash)) {
              type = COT::Division;
            } else if (expect(TokenType::Caret)) {
              type = COT::Xor;
            } else if (expect(TokenType::LeftShift)) {
              type = COT::LeftShift;
            } else if (expect(TokenType::ClosingAngleBracket)) {
              if (expect(TokenType::ClosingAngleBracket)) {
                type = COT::RightShift;
              } else {
                type = COT::GreaterThan;
              }
            } else if (expect(TokenType::Ampersand)) {
              type = COT::BitAnd;
            } else if (expect(TokenType::Pipe)) {
              type = COT::BitOr;
            } else if (expect(TokenType::Or)) {
              type = COT::Or;
            } else if (expect(TokenType::And)) {
              type = COT::And;
            } else if (expect(TokenType::Equality)) {
              type = COT::Equality;
            } else if (expect(TokenType::Inequality)) {
              type = COT::Inequality;
            } else if (expect(TokenType::OpeningAngleBracket)) {
              type = COT::LessThan;
            } else if (expect(TokenType::LessThanOrEqualTo)) {
              type = COT::LessThanOrEqualTo;
            } else if (expect(TokenType::GreaterThanOrEqualTo)) {
              type = COT::GreaterThanOrEqualTo;
            } else if (leftThis && expect(TokenType::EqualSign)) {
              type = COT::SimpleAssignment;
            }

            AC_AS_OP_SYM(Plus, Addition)
            AC_AS_OP_SYM(Minus, Subtraction)
            AC_AS_OP_SYM(Times, Multiplication)
            AC_AS_OP_SYM(Divided, Division)
            AC_AS_OP_DSYM(Modulo)
            AC_AS_OP_DSYM(LeftShift)
            AC_AS_OP_DSYM(RightShift)
            AC_AS_OP_DSYM(BitwiseAnd)
            AC_AS_OP_DSYM(BitwiseOr)
            AC_AS_OP_DSYM(BitwiseXor)

            #undef AC_AS_OP_SYM
            #undef AC_AS_OP_DSYM

            if (type == COT::NONE) ACP_NOT_OK;

            auto op = std::dynamic_pointer_cast<AST::ClassOperatorDefinitionStatement>(ruleNode);

            op->type = type;

            if (state.internalIndex == 6) {
              state.internalIndex = 9;
              ACP_RULE(Type);
            } else {
              if (!expectKeyword("this")) ACP_NOT_OK;
              state.internalIndex = 3;
              ACP_RULE(NullRule);
            }
          } else if (state.internalIndex == 8 || state.internalIndex == 9) {
            if (!exps.back()) ACP_NOT_OK;

            auto op = std::dynamic_pointer_cast<AST::ClassOperatorDefinitionStatement>(ruleNode);

            op->argumentType = std::dynamic_pointer_cast<AST::Type>(*exps.back().item);

            if (state.internalIndex == 8) {
              state.internalIndex = 7;
              ACP_RULE(NullRule);
            } else {
              state.internalIndex = 3;
              ACP_RULE(NullRule);
            }
          } else if (state.internalIndex == 10) {
            if (!exps.back()) ACP_NOT_OK;

            auto op = std::dynamic_pointer_cast<AST::ClassOperatorDefinitionStatement>(ruleNode);

            op->argumentType = std::dynamic_pointer_cast<AST::Type>(*exps.back().item);

            if (!expect(TokenType::ClosingSquareBracket)) ACP_NOT_OK;

            state.internalIndex = 3;
            ACP_RULE(NullRule);
          }
        } else if (rule == RuleType::Enumeration) {
          if (state.internalIndex == 0) {
            auto enumer = nodeFactory.create<AST::EnumerationDefinitionNode>();
            enumer->modifiers = expectModifiers(ModifierTargetType::Enumeration);

            if (!expectKeyword("enum")) ACP_NOT_OK;

            auto id = expect(TokenType::Identifier);
            if (!id) ACP_NOT_OK;
            enumer->name = id.raw;

            if (!expect(TokenType::Colon)) ACP_NOT_OK;
            state.internalIndex = 4;
            ruleNode = std::move(enumer);
            ACP_RULE(Type);
          } else if (state.internalIndex == 1 || state.internalIndex == 2) {
            auto enumer = std::dynamic_pointer_cast<AST::EnumerationDefinitionNode>(ruleNode);
            if (state.internalIndex == 2) {
              auto key = ALTACORE_ANY_CAST<std::string>(state.internalValue);
              enumer->members.back().second = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);
              state.internalIndex = expect(TokenType::Comma) ? 1 : 3;
            }
            while (state.internalIndex == 1) {
              auto name = expect(TokenType::Identifier);
              if (!name) {
                state.internalIndex = 3;
                break;
              }
              enumer->members.push_back(std::make_pair(name.raw, nullptr));
              if (expect(TokenType::Comma)) {
                continue;
              } else if (expect(TokenType::EqualSign)) {
                state.internalValue = name.raw;
                state.internalIndex = 2;
                break;
              } else {
                state.internalIndex = 3;
                break;
              }
            }
            if (state.internalIndex == 2) {
              ACP_RULE(Expression);
            } else if (state.internalIndex == 3) {
              if (!expect(TokenType::ClosingBrace)) ACP_NOT_OK;
              ACP_NODE(ruleNode);
            }
          } else if (state.internalIndex == 4) {
            auto enumer = std::dynamic_pointer_cast<AST::EnumerationDefinitionNode>(ruleNode);

            if (!exps.back()) ACP_NOT_OK;

            enumer->underlyingType = std::dynamic_pointer_cast<AST::Type>(*exps.back().item);

            if (!expect(TokenType::OpeningBrace)) ACP_NOT_OK;

            state.internalIndex = 1;
            ACP_RULE(NullRule);
          }
        } else if (rule == RuleType::Yield) {
          if (state.internalIndex == 0) {
            state.internalIndex = expectKeyword("yield") ? 1 : 2;
            ACP_RULE(PunctualConditonalExpression);
          } else if (state.internalIndex == 1) {
            auto node = nodeFactory.create<AST::YieldExpression>();

            if (exps.back()) {
              node->target = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);
            }

            ACP_NODE(node);
          } else if (state.internalIndex == 2) {
            ACP_EXP(exps.back().item);
          }
        } else if (rule == RuleType::Assertion) {
          if (state.internalIndex == 0) {
            if (!expectKeyword("assert")) ACP_NOT_OK;
            state.internalIndex = 1;
            ACP_RULE(Expression);
          } else {
            if (!exps.back()) ACP_NOT_OK;

            auto node = nodeFactory.create<AST::AssertionStatement>();

            node->test = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);

            ACP_NODE(node);
          }
        } else if (rule == RuleType::Await) {
          if (state.internalIndex == 0) {
            saveState();
            state.internalIndex = (expectKeyword("await")) ? 1 : 2;
            ACP_RULE(NotOrPointerOrDereferenceOrPreIncDecOrPlusMinusOrBitNot);
          } else if (state.internalIndex == 1) {
            if (!exps.back()) {
              exps.pop_back();
              restoreState();
              state.internalIndex = 2;
              ACP_RULE(NotOrPointerOrDereferenceOrPreIncDecOrPlusMinusOrBitNot);
            }

            auto await = nodeFactory.create<AST::AwaitExpression>();
            await->target = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);
            ACP_NODE(await);
          } else {
            ACP_EXP(exps.back().item);
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
