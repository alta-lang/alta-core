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

    bool Parser::expectBinaryOperation(RuleType rule, RuleType nextHigherPrecedentRule, std::vector<TokenType> operatorTokens, std::vector<AST::OperatorType> operatorTypes, RuleState& state, std::vector<Expectation>& exps, NextFunctionType next) {
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

        state.internalValue = std::make_pair(currentState, std::move(binOp));
        state.internalIndex = 2;

        next(true, { nextHigherPrecedentRule }, nullptr);
        return true;
      } else {
        auto [savedState, binOp] = ALTACORE_ANY_CAST<std::pair<decltype(currentState), std::shared_ptr<AST::BinaryOperation>>>(state.internalValue);

        if (!exps.back()) {
          if (state.internalIndex == 2) {
            next(false, {}, nullptr);
            return true;
          } else {
            currentState = savedState;
            next(true, {}, binOp->left);
            return true;
          }
        }

        binOp->right = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);

        Token opExp = expect(operatorTokens);

        if (opExp) {
          auto savedState = currentState;
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

          state.internalValue = std::make_pair(savedState, std::move(otherBinOp));
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
        std::vector<PrepoExpectation>()
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

        auto& [newRule, newNextExps, newRuleState, newExps] = ruleStack.top();

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

      auto currentLine = peek(1, true).line;

      auto expect = [&](TokenType type) -> Token {
        auto invalidToken = Token();
        invalidToken.valid = false;
        if (peek().line != currentLine) return invalidToken;
        return this->expect({ type }, true);
      };

      while (ruleStack.size() > 0) {
        auto& [rule, nextExps, state, exps] = ruleStack.top();

        if (nextExps.size() > 0) {
          auto nextExp = nextExps.top();
          nextExps.pop();

          ruleStack.emplace(
            nextExp,
            std::stack<PrepoRuleType>(),
            RuleState(currentState),
            std::vector<PrepoExpectation>()
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

            auto left = (exps.size() == 1) ? ALTACORE_ANY_CAST<PrepoExpression>(state.internalValue) : *exps.front().item;
            auto result = evaluateExpressions ? PrepoExpression(left == *exps.back().item) : PrepoExpression();

            exps.clear();

            if (expect(TokenType::Equality)) {
              state.internalValue = std::move(result);
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
            state.internalValue = std::make_tuple(target.raw, std::vector<PrepoExpression>());
            PREPO_RULE(Expression);
          } else if (state.internalIndex == 1) {
            auto [target, args] = ALTACORE_ANY_CAST<std::tuple<std::string, std::vector<PrepoExpression>>>(state.internalValue);

            if (exps.back()) {
              args.push_back(*exps.back().item);
              if (expect(TokenType::Comma)) {
                PREPO_RULE(Expression);
              }
            }

            if (!expect(TokenType::ClosingParenthesis)) PREPO_NOT_OK;

            if (!evaluateExpressions) PREPO_NODE(PrepoExpression());

            if (target == "defined") {
              PREPO_NODE(Prepo::defined(args));
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

            auto left = (exps.size() == 1) ? ALTACORE_ANY_CAST<PrepoExpression>(state.internalValue) : *exps.front().item;
            auto result = evaluateExpressions ? PrepoExpression(left && *exps.back().item) : PrepoExpression();
            if (!result) {
              evaluateExpressions = false;
            }

            exps.clear();

            if (expect(TokenType::And)) {
              state.internalValue = std::move(result);
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

            auto left = (exps.size() == 1) ? ALTACORE_ANY_CAST<PrepoExpression>(state.internalValue) : *exps.front().item;
            auto result = evaluateExpressions ? PrepoExpression(left || *exps.back().item) : PrepoExpression();
            if (result) {
              evaluateExpressions = false;
            }

            exps.clear();

            if (expect(TokenType::Or)) {
              state.internalValue = std::move(result);
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
        std::vector<Expectation>()
      );

      auto next = [&](bool ok = false, std::vector<RuleType> rules = {}, NodeType result = nullptr) {
        auto& state = std::get<2>(ruleStack.top());
        state.iteration++;

        if (result) {
          result->position.line = tokens[state.stateAtStart.currentPosition].line;
          result->position.column = tokens[state.stateAtStart.currentPosition].column;
          result->position.file = filePath;
        }

        auto& ruleExps = std::get<1>(ruleStack.top());
        for (auto it = rules.rbegin(); it != rules.rend(); it++) {
          ruleExps.push(*it);
        }

        if (ok && rules.size() > 0) return;

        auto oldRuleType = std::get<0>(ruleStack.top());
        auto oldState = std::get<2>(ruleStack.top());
        ruleStack.pop();

        if (ruleStack.size() < 1) return;

        auto& [newRule, newNextExps, newRuleState, newExps] = ruleStack.top();

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
        auto& [rule, nextExps, state, exps] = ruleStack.top();

        if (advanceExp && nextExps.size() > 0) {
          auto nextExp = nextExps.top();
          nextExps.pop();

          ruleStack.emplace(
            nextExp,
            std::stack<RuleType>(),
            RuleState(currentState),
            std::vector<Expectation>()
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
            RuleType::WhileLoop,
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

            state.internalValue = std::move(funcDef);
            state.internalIndex = 2;
            ACP_RULE(Attribute);
          } else if (state.internalIndex == 2) {
            if (exps.back()) ACP_RULE(Attribute);

            exps.pop_back();

            auto funcDef = ALTACORE_ANY_CAST<std::shared_ptr<AST::FunctionDefinitionNode>>(state.internalValue);

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
            auto funcDef = ALTACORE_ANY_CAST<std::shared_ptr<AST::FunctionDefinitionNode>>(state.internalValue);

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
            auto funcDef = ALTACORE_ANY_CAST<std::shared_ptr<AST::FunctionDefinitionNode>>(state.internalValue);

            if (!exps.back()) ACP_NOT_OK;
            funcDef->returnType = std::dynamic_pointer_cast<AST::Type>(*exps.back().item);

            state.internalIndex = 5;
            ACP_RULE(Block);
          } else {
            auto funcDef = ALTACORE_ANY_CAST<std::shared_ptr<AST::FunctionDefinitionNode>>(state.internalValue);

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

            state.internalValue = std::move(param);
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

            auto param = ALTACORE_ANY_CAST<std::shared_ptr<AST::Parameter>>(state.internalValue);

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
          } else {
            if (!exps.back()) ACP_NOT_OK;

            if (!expect(TokenType::Dot)) ACP_EXP(exps.back().item);

            auto query = expect(TokenType::Identifier);
            if (!query) ACP_NOT_OK;

            auto acc = std::make_shared<AST::Accessor>(std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item), query.raw);

            auto state = currentState;
            while (expect(TokenType::Dot)) {
              query = expect(TokenType::Identifier);
              if (!query) {
                currentState = state;
                break;
              }
              acc = std::make_shared<AST::Accessor>(acc, query.raw);
              state = currentState;
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

            state.internalValue = std::move(type);

            if (!expect(TokenType::OpeningParenthesis)) {
              state.internalIndex = 3;
              ACP_RULE(StrictAccessor);
            }

            state.internalIndex = 1;

            ACP_RULE(Type);
          } else if (state.internalIndex == 1) {
            auto type = ALTACORE_ANY_CAST<std::shared_ptr<AST::Type>>(state.internalValue);

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

            auto type = ALTACORE_ANY_CAST<std::shared_ptr<AST::Type>>(state.internalValue);

            type->returnType = std::dynamic_pointer_cast<AST::Type>(*exps.back().item);
            ACP_NODE(type);
          } else {
            auto type = ALTACORE_ANY_CAST<std::shared_ptr<AST::Type>>(state.internalValue);

            if (!exps.back()) ACP_NOT_OK;

            auto expr = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);

            if (expr->nodeType() == AST::NodeType::Fetch) {
              auto id = std::dynamic_pointer_cast<AST::Fetch>(expr);
              auto name = id->query;
              if (name == "int" || name == "byte" || name == "char" || name == "bool" || name == "void") {
                if (typesToIgnore.find(name) != typesToIgnore.end()) ACP_NOT_OK;
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
            state.internalValue = std::make_shared<AST::BlockNode>();

            ACP_RULE(Statement);
          } else {
            auto block = ALTACORE_ANY_CAST<std::shared_ptr<AST::BlockNode>>(state.internalValue);

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

            state.internalValue = std::move(varDef);

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
            auto varDef = ALTACORE_ANY_CAST<std::shared_ptr<AST::VariableDefinitionExpression>>(state.internalValue);
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
            auto varDef = ALTACORE_ANY_CAST<std::shared_ptr<AST::VariableDefinitionExpression>>(state.internalValue);
            varDef->initializationExpression = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);

            ACP_NODE(varDef);
          }
        } else if (rule == RuleType::Fetch) {
          auto id = expect(TokenType::Identifier);
          if (!id) ACP_NOT_OK;
          ACP_NODE(std::make_shared<AST::Fetch>(id.raw));
        } else if (rule == RuleType::Accessor) {
          if (state.internalIndex == 0) {
            state.internalIndex = 1;
            if (inClass) {
              ACP_RULE_LIST(
                RuleType::SuperClassFetch,
                RuleType::Fetch,
                RuleType::GroupedExpression
              );
            } else {
              ACP_RULE_LIST(
                RuleType::Fetch,
                RuleType::GroupedExpression
              );
            }
          } else {
            if (!exps.back()) ACP_NOT_OK;

            if (!expect(TokenType::Dot)) ACP_EXP(exps.back().item);

            auto query = expect(TokenType::Identifier);
            if (!query) ACP_NOT_OK;

            auto acc = std::make_shared<AST::Accessor>(std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item), query.raw);

            auto state = currentState;
            while (expect(TokenType::Dot)) {
              query = expect(TokenType::Identifier);
              if (!query) {
                currentState = state;
                break;
              }
              acc = std::make_shared<AST::Accessor>(acc, query.raw);
              state = currentState;
            }

            ACP_NODE(acc);
          }
        } else if (rule == RuleType::Assignment) {
          if (state.internalIndex == 0) {
            state.internalIndex = 1;
            ACP_RULE(VerbalConditionalExpression);
          } else if (state.internalIndex == 1) {
            if (!exps.back()) ACP_NOT_OK;

            if (!expect(TokenType::EqualSign)) ACP_EXP(exps.back().item);

            state.internalValue = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);
            state.internalIndex = 2;

            ACP_RULE(Assignment);
          } else {
            if (!exps.back()) ACP_NOT_OK;

            auto lhs = ALTACORE_ANY_CAST<std::shared_ptr<AST::ExpressionNode>>(state.internalValue);

            ACP_NODE((std::make_shared<AST::AssignmentExpression>(lhs, std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item))));
          }
        } else if (rule == RuleType::AdditionOrSubtraction) {
          if (expectBinaryOperation(rule, RuleType::MultiplicationOrDivision, {
            TokenType::PlusSign,
            TokenType::MinusSign,
          }, {
            AST::OperatorType::Addition,
            AST::OperatorType::Subtraction,
          }, state, exps, next)) {
            continue;
          }
        } else if (rule == RuleType::MultiplicationOrDivision) {
          if (expectBinaryOperation(rule, RuleType::Cast, {
            TokenType::Asterisk,
            TokenType::ForwardSlash,
          }, {
            AST::OperatorType::Multiplication,
            AST::OperatorType::Division,
          }, state, exps, next)) {
            continue;
          }
        } else if (rule == RuleType::ModuleOnlyStatement) {
          if (state.iteration == 0) {
            ACP_RULE(Import);
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
        } else if (rule == RuleType::FunctionCallOrSubscript) {
          if (state.internalIndex == 0) {
            state.internalIndex = 1;
            ACP_RULE(ClassInstantiation);
          } else if (state.internalIndex == 1) {
            if (!exps.back()) ACP_NOT_OK;

            bool isSubscript = false;

            if (expect(TokenType::OpeningSquareBracket)) isSubscript = true;
            if (!isSubscript && !expect(TokenType::OpeningParenthesis)) ACP_EXP(exps.back().item);

            if (!isSubscript) {
              auto funcCall = std::make_shared<AST::FunctionCallExpression>();
              funcCall->target = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);

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

              state.internalValue = std::move(funcCall);
              state.internalIndex = 2;

              ACP_RULE(Expression);
            } else {
              auto subs = std::make_shared<AST::SubscriptExpression>();
              subs->target = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);

              state.internalValue = std::move(subs);
              state.internalIndex = 3;
              
              ACP_RULE(Expression);
            }
          } else if (state.internalIndex == 2) {
            auto callState = ALTACORE_ANY_CAST<std::shared_ptr<AST::FunctionCallExpression>>(state.internalValue);

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

            if (expect(TokenType::OpeningParenthesis)) {
              auto newCall = std::make_shared<AST::FunctionCallExpression>(callState);

              auto tmpState = currentState;
              auto name = expect(TokenType::Identifier);
              if (name && !expect(TokenType::Colon)) {
                name = Token(); // constructs an invalid expectation
                currentState = tmpState;
              }
              newCall->arguments.push_back({
                (name) ? name.raw : "",
                nullptr,
              });
              state.internalValue = std::move(newCall);
              ACP_RULE(Expression);
            } else if (expect(TokenType::OpeningSquareBracket)) {
              auto subs = std::make_shared<AST::SubscriptExpression>();
              subs->target = callState;
              
              state.internalValue = std::move(subs);
              state.internalIndex = 3;

              ACP_RULE(Expression);
            }

            ACP_NODE((callState));
          } else if (state.internalIndex == 3) {
            auto subs = ALTACORE_ANY_CAST<std::shared_ptr<AST::SubscriptExpression>>(state.internalValue);

            if (!exps.back()) ACP_NOT_OK; // TODO: error recovery

            if (!expect(TokenType::ClosingSquareBracket)) ACP_NOT_OK;

            subs->index = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);

            if (expect(TokenType::OpeningParenthesis)) {
              auto call = std::make_shared<AST::FunctionCallExpression>(subs);

              auto tmpState = currentState;
              auto name = expect(TokenType::Identifier);
              if (name && !expect(TokenType::Colon)) {
                name = Token(); // constructs an invalid expectation
                currentState = tmpState;
              }
              call->arguments.push_back({
                (name) ? name.raw : "",
                nullptr,
              });
              state.internalValue = std::move(call);
              state.internalIndex = 2;

              ACP_RULE(Expression);
            } else if (expect(TokenType::OpeningSquareBracket)) {
              auto newSubs = std::make_shared<AST::SubscriptExpression>();
              newSubs->target = subs;
              
              state.internalValue = std::move(newSubs);

              ACP_RULE(Expression);
            }

            ACP_NODE((subs));
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

            state.internalValue = std::move(funcDecl);
            state.internalIndex = 1;
            ACP_RULE(Parameter);
          } else if (state.internalIndex == 1) {
            auto funcDecl = ALTACORE_ANY_CAST<std::shared_ptr<AST::FunctionDeclarationNode>>(state.internalValue);

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
            auto funcDecl = ALTACORE_ANY_CAST<std::shared_ptr<AST::FunctionDeclarationNode>>(state.internalValue);

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
              state.internalValue = std::move(attr);

              ACP_RULE(AnyLiteral);
            } else {
              ACP_NODE((std::move(attr)));
            }
          } else {
            auto attr = ALTACORE_ANY_CAST<std::shared_ptr<AST::AttributeNode>>(state.internalValue);

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
              RuleType::IntegralLiteral,
              RuleType::BooleanLiteral,
              RuleType::String,
              RuleType::Character
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

            state.internalValue = ConditionStatementState(currentState, std::move(cond));
            state.internalIndex = 2;

            ACP_RULE(Statement);
          } else if (state.internalIndex == 2) {
            if (!exps.back()) ACP_NOT_OK;

            auto intern = ALTACORE_ANY_CAST<ConditionStatementState<decltype(currentState)>>(state.internalValue);

            intern.cond->primaryResult = std::dynamic_pointer_cast<AST::StatementNode>(*exps.back().item);

            intern.state = currentState;
            if (expectKeyword("else")) {
              if (expectKeyword("if")) {
                state.internalIndex = 3;
                ACP_RULE(Expression);
              } else {
                state.internalIndex = 5;
                ACP_RULE(Statement);
              }
            }

            ACP_NODE((intern.cond));
          } else if (state.internalIndex == 3) {
            auto intern = ALTACORE_ANY_CAST<ConditionStatementState<decltype(currentState)>>(state.internalValue);

            if (!exps.back()) {
              currentState = intern.state;
              ACP_NODE((intern.cond));
            }

            intern.cond->alternatives.push_back({
              std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item),
              nullptr,
            });

            state.internalIndex = 4;

            ACP_RULE(Statement);
          } else if (state.internalIndex == 4) {
            auto intern = ALTACORE_ANY_CAST<ConditionStatementState<decltype(currentState)>>(state.internalValue);

            if (!exps.back()) {
              currentState = intern.state;
              intern.cond->alternatives.pop_back();
              ACP_NODE((intern.cond));
            }

            intern.cond->alternatives.back().second = std::dynamic_pointer_cast<AST::StatementNode>(*exps.back().item);

            state.internalValue = ConditionStatementState(currentState, intern.cond);
            if (expectKeyword("else")) {
              if (expectKeyword("if")) {
                state.internalIndex = 3;
                ACP_RULE(Expression);
              } else {
                state.internalIndex = 5;
                ACP_RULE(Statement);
              }
            }

            ACP_NODE((intern.cond));
          } else if (state.internalIndex == 5) {
            auto intern = ALTACORE_ANY_CAST<ConditionStatementState<decltype(currentState)>>(state.internalValue);

            if (!exps.back()) {
              currentState = intern.state;
              ACP_NODE((intern.cond));
            }

            intern.cond->finalResult = std::dynamic_pointer_cast<AST::StatementNode>(*exps.back().item);

            ACP_NODE((intern.cond));
          }
        } else if (rule == RuleType::VerbalConditionalExpression) {
          if (state.internalIndex == 0) {
            state.internalIndex = 1;
            ACP_RULE(PunctualConditonalExpression);
          } else if (state.internalIndex == 1) {
            if (!exps.back()) ACP_NOT_OK;

            State stateCache = currentState;
            if (!expectKeyword("if")) ACP_EXP(exps.back().item);

            auto cond = std::make_shared<AST::ConditionalExpression>();
            cond->primaryResult = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);

            state.internalValue = VerbalConditionalState<State>(std::move(cond), stateCache);
            state.internalIndex = 2;

            ACP_RULE(Expression);
          } else if (state.internalIndex == 2) {
            auto ruleState = ALTACORE_ANY_CAST<VerbalConditionalState<State>>(state.internalValue);

            if (!exps.back()) {
              currentState = ruleState.stateCache;
              ACP_NODE((ruleState.cond->primaryResult));
            }

            if (!expectKeyword("else")) {
              currentState = ruleState.stateCache;
              ACP_NODE((ruleState.cond->primaryResult));
            }

            ruleState.cond->test = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);

            state.internalIndex = 3;
            ACP_RULE(PunctualConditonalExpression);
          } else if (state.internalIndex == 3) {
            auto ruleState = ALTACORE_ANY_CAST<VerbalConditionalState<State>>(state.internalValue);

            if (!exps.back()) {
              currentState = ruleState.stateCache;
              ACP_NODE((ruleState.cond->primaryResult));
            }

            ruleState.cond->secondaryResult = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);

            State stateCache = currentState;
            if (expectKeyword("if")) {
              auto newCond = std::make_shared<AST::ConditionalExpression>();
              newCond->primaryResult = ruleState.cond;

              state.internalValue = VerbalConditionalState<State>(std::move(newCond), stateCache, true);
              state.internalIndex = 2;

              ACP_RULE(Expression);
            }

            ACP_NODE((ruleState.cond));
          }
        } else if (rule == RuleType::PunctualConditonalExpression) {
          if (state.internalIndex == 0) {
            state.internalIndex = 1;
            ACP_RULE(EqualityRelationalOperation);
          } else if (state.internalIndex == 1) {
            if (!exps.back()) ACP_NOT_OK;

            if (!expect(TokenType::QuestionMark)) ACP_EXP(exps.back().item);

            auto cond = std::make_shared<AST::ConditionalExpression>();
            cond->test = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);

            state.internalIndex = 2;
            state.internalValue = std::move(cond);

            ACP_RULE(Expression);
          } else if (state.internalIndex == 2) {
            if (!exps.back()) ACP_NOT_OK;

            if (!expect(TokenType::Colon)) ACP_NOT_OK;

            auto cond = ALTACORE_ANY_CAST<std::shared_ptr<AST::ConditionalExpression>>(state.internalValue);
            cond->primaryResult = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);

            state.internalIndex = 3;

            ACP_RULE(PunctualConditonalExpression);
          } else if (state.internalIndex == 3) {
            if (!exps.back()) ACP_NOT_OK;

            auto cond = ALTACORE_ANY_CAST<std::shared_ptr<AST::ConditionalExpression>>(state.internalValue);
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
          }, state, exps, next)) {
            continue;
          }
        } else if (rule == RuleType::EqualityRelationalOperation) {
          if (expectBinaryOperation(rule, RuleType::NonequalityRelationalOperation, {
            TokenType::Equality,
            TokenType::Inequality,
          }, {
            AST::OperatorType::EqualTo,
            AST::OperatorType::NotEqualTo,
          }, state, exps, next)) {
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

            state.internalValue = std::move(def);
            state.internalIndex = 1;
            ACP_RULE(ClassStatement);
          } else {
            auto klass = ALTACORE_ANY_CAST<std::shared_ptr<AST::ClassDefinitionNode>>(state.internalValue);

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

            state.internalValue = std::make_shared<AST::ClassMemberDefinitionStatement>(AST::parseVisibility(*visibilityMod));
            state.internalIndex = 1;
            ACP_RULE(VariableDefinition);
          } else {
            if (!exps.back()) ACP_NOT_OK;

            auto memberDef = ALTACORE_ANY_CAST<std::shared_ptr<AST::ClassMemberDefinitionStatement>>(state.internalValue);
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

            state.internalValue = std::make_pair(attrs, std::make_shared<AST::ClassMethodDefinitionStatement>(AST::parseVisibility(*visibilityMod)));
            state.internalIndex = 2;
            inClass = true;
            ACP_RULE(FunctionDefinition);
          } else {
            inClass = false;
            if (!exps.back()) ACP_NOT_OK;

            auto [attrs, methodDef] = ALTACORE_ANY_CAST<std::pair<std::vector<std::shared_ptr<AST::AttributeNode>>, std::shared_ptr<AST::ClassMethodDefinitionStatement>>>(state.internalValue);
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

            state.internalValue = std::move(method);
            state.internalIndex = 2;

            ACP_RULE(Attribute);
          } else if (state.internalIndex == 2) {
            if (exps.back()) {
              ACP_RULE(Attribute);
            } else {
              exps.pop_back();
            }

            auto method = ALTACORE_ANY_CAST<std::shared_ptr<AST::ClassSpecialMethodDefinitionStatement>>(state.internalValue);

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
            auto method = ALTACORE_ANY_CAST<std::shared_ptr<AST::ClassSpecialMethodDefinitionStatement>>(state.internalValue);

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

            auto method = ALTACORE_ANY_CAST<std::shared_ptr<AST::ClassSpecialMethodDefinitionStatement>>(state.internalValue);
            method->body = std::dynamic_pointer_cast<AST::BlockNode>(*exps.back().item);

            ACP_NODE((method));
          }
        } else if (rule == RuleType::ClassInstantiation) {
          if (state.internalIndex == 0) {
            state.internalValue = currentState;
            if (!expectKeyword("new")) {
              state.internalIndex = 4;
              if (inClass) {
                state.internalIndex = 5;
                ACP_RULE(SuperClassFetch);
              }
              ACP_RULE_LIST(
                RuleType::BooleanLiteral,
                RuleType::IntegralLiteral,
                RuleType::String,
                RuleType::Character,
                RuleType::Accessor
              );
            }
            state.internalIndex = 1;
            ACP_RULE(Accessor);
          } else if (state.internalIndex == 1 || state.internalIndex == 5) {
            if (!exps.back()) {
              if (state.internalIndex != 5) {
                currentState = ALTACORE_ANY_CAST<decltype(currentState)>(state.internalValue);
              }
              state.internalIndex = 4;
              ACP_RULE_LIST(
                RuleType::BooleanLiteral,
                RuleType::IntegralLiteral,
                RuleType::String,
                RuleType::Character,
                RuleType::Accessor
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
              state.internalValue = std::move(inst);
              ACP_RULE(Expression);
            } else if (isSuperclassFetch) {
              currentState = ALTACORE_ANY_CAST<decltype(currentState)>(state.internalValue);
              state.internalIndex = 4;
              ACP_RULE_LIST(
                RuleType::BooleanLiteral,
                RuleType::IntegralLiteral,
                RuleType::String,
                RuleType::Character,
                RuleType::Accessor
              );
            }

            ACP_NODE((inst));
          } else if (state.internalIndex == 2) {
            auto inst = ALTACORE_ANY_CAST<std::shared_ptr<AST::ClassInstantiationExpression>>(state.internalValue);

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
            ACP_RULE(PointerOrDereference);
          } else if (state.internalIndex == 1) {
            if (!exps.back()) ACP_NOT_OK;
            auto cast = std::make_shared<AST::CastExpression>();
            cast->target = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);
            state.internalValue = std::make_pair(std::move(cast), currentState);
            state.internalIndex = 2;
            if (!expectKeyword("as")) ACP_EXP(exps.back().item);
            ACP_RULE(Type);
          } else {
            auto cast = ALTACORE_ANY_CAST<std::pair<std::shared_ptr<AST::CastExpression>, decltype(currentState)>>(state.internalValue);
            if (!exps.back()) {
              currentState = cast.second;
              ACP_NODE((cast.first->target));
            }
            cast.first->type = std::dynamic_pointer_cast<AST::Type>(*exps.back().item);
            ACP_NODE((cast.first));
          }
        } else if (rule == RuleType::PointerOrDereference) {
          if (state.internalIndex == 0) {
            if (expect(TokenType::Asterisk) || expectKeyword("valueof")) {
              state.internalIndex = 2;
            } else if (expect(TokenType::Ampersand) || expectKeyword("getptr")) {
              state.internalIndex = 3;
            } else {
              state.internalIndex = 1;
              ACP_RULE(FunctionCallOrSubscript);
            }

            ACP_RULE(PointerOrDereference);
          } else if (state.internalIndex == 1) {
            ACP_EXP(exps.back().item);
          } else {
            if (!exps.back()) ACP_NOT_OK;

            auto expr = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);

            if (state.internalIndex == 2) {
              auto val = std::make_shared<AST::DereferenceExpression>();
              val->target = expr;
              ACP_NODE((val));
            } else if (state.internalIndex == 3) {
              auto val = std::make_shared<AST::PointerExpression>();
              val->target = expr;
              ACP_NODE((val));
            } else {
              ACP_NOT_OK;
            }
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
            
            state.internalValue = std::move(loop);
            state.internalIndex = 2;
            ACP_RULE(Statement);
          } else {
            if (!exps.back()) ACP_NOT_OK;

            auto loop = ALTACORE_ANY_CAST<std::shared_ptr<AST::WhileLoopStatement>>(state.internalValue);
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
            state.internalValue = std::move(typeAlias);
            state.internalIndex = 1;
            ACP_RULE(Type);
          } else {
            if (!exps.back()) ACP_NOT_OK;
            auto typeAlias = ALTACORE_ANY_CAST<std::shared_ptr<AST::TypeAliasStatement>>(state.internalValue);
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
                state.internalValue = std::move(sup);
                ACP_RULE(StrictAccessor);
              }
            }

            ACP_NODE((sup));
          } else {
            auto sup = ALTACORE_ANY_CAST<std::shared_ptr<AST::SuperClassFetch>>(state.internalValue);

            if (!exps.back()) ACP_NOT_OK;
            sup->fetch = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);
            if (!expect(TokenType::ClosingAngleBracket)) ACP_NOT_OK;

            ACP_NODE((sup));
          }
        } else if (rule == RuleType::Instanceof) {
          if (state.internalIndex == 0) {
            state.internalIndex = 1;
            ACP_RULE(AdditionOrSubtraction);
          } else if (state.internalIndex == 1) {
            if (!exps.back()) ACP_NOT_OK;
            if (!expectKeyword("instanceof")) ACP_EXP(exps.back().item);
            auto instOf = std::make_shared<AST::InstanceofExpression>();
            instOf->target = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);
            state.internalValue = std::move(instOf);
            state.internalIndex = 2;
            ACP_RULE(Type);
          } else {
            auto instOf = ALTACORE_ANY_CAST<std::shared_ptr<AST::InstanceofExpression>>(state.internalValue);
            if (!exps.back()) ACP_NOT_OK;
            instOf->type = std::dynamic_pointer_cast<AST::Type>(*exps.back().item);
            ACP_NODE((instOf));
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
