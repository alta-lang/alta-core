#include <algorithm>
#include "../include/altacore/parser.hpp"

namespace AltaCore {
  namespace Parser {
    Expectation Parser::expect(std::initializer_list<ExpectationType> expectations) {
      Expectation ret; // by default, Expectations are invalid
      const State stateAtStart = currentState;
      State state = stateAtStart;

      if (failed.find(stateAtStart.currentPosition) == failed.end()) {
        failed[stateAtStart.currentPosition] = std::vector<RuleType>();
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
          if (tmp != nullptr) {
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
    Expectation Parser::expect(ExpectationType expectation) {
      return expect({ expectation });
    };

    // <helper-functions>
    std::vector<std::shared_ptr<AST::Parameter>> Parser::expectParameters() {
      std::vector<std::shared_ptr<AST::Parameter>> parameters;
      Expectation param;
      param = expect(RuleType::Parameter);
      if (param.valid) {
        do {
          std::shared_ptr<AST::Parameter> parameter = std::dynamic_pointer_cast<AST::Parameter>(param.item);
          if (parameter == nullptr) throw std::runtime_error("oh no.");
          parameters.push_back(parameter);
          State state = currentState;
          if (!expect(TokenType::Comma).valid) break;
          param = expect(RuleType::Parameter);
          if (!param.valid) currentState = state;
        } while (param.valid);
      }
      return parameters;
    };
    std::vector<std::string> Parser::expectModifiers(ModifierTargetType mtt) {
      std::vector<std::string> modifiers;
      Expectation mod;
      State state;
      while ((state = currentState), (mod = expect(TokenType::Identifier)), mod.valid) {
        bool cont = false;
        for (auto& modifier: modifiersForTargets[(unsigned int)mtt]) {
          if (mod.token.raw == modifier) {
            modifiers.push_back(mod.token.raw);
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
    // </helper-functions>

    std::shared_ptr<AST::Node> Parser::runRule(RuleType rule) {
      if (rule == RuleType::Root) {
        // TODO: use custom `ParserError`s instead of throwing `std::runtime_error`s
        std::vector<std::shared_ptr<AST::StatementNode>> statements;
        Expectation exp;
        while ((exp = expect(RuleType::Statement)), exp.valid) {
          auto stmt = std::dynamic_pointer_cast<AST::StatementNode>(exp.item);
          if (stmt == nullptr) throw std::runtime_error("AST node given was not of the expected type");
          statements.push_back(stmt);
        }
        if (currentState.currentPosition < tokens.size()) {
          throw std::runtime_error("Oof."); // TODO: better error message. i'm just lazy right now
        }
        return std::make_shared<AST::RootNode>(statements);
      } else if (rule == RuleType::Statement) {
        auto exp = expect({
          RuleType::FunctionDefinition,
          RuleType::ReturnDirective,
          RuleType::Expression,
        });
        expect(TokenType::Semicolon); // optional
        if (!exp.valid) return nullptr;
        auto ret = std::dynamic_pointer_cast<AST::StatementNode>(exp.item);
        if (!exp.type.isToken && exp.type.rule == RuleType::Expression) {
          auto expr = std::dynamic_pointer_cast<AST::ExpressionNode>(exp.item);
          if (expr == nullptr) throw std::runtime_error("wtf");
          ret = std::make_shared<AST::ExpressionStatement>(expr);
        }
        return ret;
      } else if (rule == RuleType::Expression) {
        auto exp = expect({
          RuleType::IntegralLiteral,
          RuleType::VariableDefinition,
          RuleType::Assignment,
          RuleType::Accessor,
          RuleType::Fetch,
        });
        return exp.item;
      } else if (rule == RuleType::FunctionDefinition) {
        auto modifiers = expectModifiers(ModifierTargetType::Function);
        auto $function = expect(TokenType::Identifier);
        if (!($function.valid && $function.token.raw == "function")) return nullptr;
        auto name = expect(TokenType::Identifier);
        if (!name.valid) return nullptr;
        if (!expect(TokenType::OpeningParenthesis).valid) return nullptr;
        auto parameters = expectParameters();
        if (!expect(TokenType::ClosingParenthesis).valid) return nullptr;
        if (!expect(TokenType::Colon).valid) return nullptr;
        auto returnType = expect(RuleType::Type);
        if (!returnType.valid) return nullptr;
        if (!expect(TokenType::OpeningBrace).valid) return nullptr;
        std::vector<std::shared_ptr<AST::StatementNode>> statements;
        Expectation stmt;
        while ((stmt = expect(RuleType::Statement)), stmt.valid) {
          auto statement = std::dynamic_pointer_cast<AST::StatementNode>(stmt.item);
          if (statement == nullptr) throw std::runtime_error("uhm...");
          statements.push_back(statement);
        }
        if (!expect(TokenType::ClosingBrace).valid) return nullptr;
        return std::make_shared<AST::FunctionDefinitionNode>(name.token.raw, parameters, std::dynamic_pointer_cast<AST::Type>(returnType.item), modifiers, std::make_shared<AST::BlockNode>(statements));
      } else if (rule == RuleType::Parameter) {
        auto name = expect(TokenType::Identifier);
        if (!name.valid) return nullptr;
        if (!expect(TokenType::Colon).valid) return nullptr;
        auto type = expect(RuleType::Type);
        if (!type.valid) return nullptr;
        auto actualType = std::dynamic_pointer_cast<AST::Type>(type.item);
        auto actualName = name.token.raw;
        return std::make_shared<AST::Parameter>(actualName, actualType);
      } else if (rule == RuleType::Type) {
        auto modifiers = expectModifiers(ModifierTargetType::Type);
        auto name = expect(TokenType::Identifier);
        std::vector<uint8_t> modifierBitflags;
        modifierBitflags.push_back(0);
        for (auto& modifier: modifiers) {
          auto& bitFlag = modifierBitflags.back();
          if (modifier == "ptr") {
            bitFlag |= (uint8_t)AST::TypeModifierFlag::Pointer;
            modifierBitflags.push_back(0);
          } else if (modifier == "const") {
            bitFlag |= (uint8_t)AST::TypeModifierFlag::Constant;
          } else if (modifier == "ref") {
            bitFlag |= (uint8_t)AST::TypeModifierFlag::Reference;
            modifierBitflags.push_back(0);
          }
        }
        if (modifierBitflags.back() == 0) {
          modifierBitflags.pop_back();
        }
        if (!name.valid) return nullptr;
        return std::make_shared<AST::Type>(name.token.raw, modifierBitflags);
      } else if (rule == RuleType::IntegralLiteral) {
        auto integer = expect(TokenType::Integer);
        if (!integer.valid) return nullptr;
        return std::make_shared<AST::IntegerLiteralNode>(integer.token.raw);
      } else if (rule == RuleType::ReturnDirective) {
        auto keyword = expect(TokenType::Identifier);
        if (!(keyword.valid && keyword.token.raw == "return")) return nullptr;
        rulesToIgnore.push_back(RuleType::ReturnDirective);
        auto expr = expect(RuleType::Expression);
        rulesToIgnore.pop_back();
        std::shared_ptr<AST::ExpressionNode> exprNode = nullptr;
        if (expr.valid) {
          exprNode = std::dynamic_pointer_cast<AST::ExpressionNode>(expr.item);
        }
        return std::make_shared<AST::ReturnDirectiveNode>(exprNode);
      } else if (rule == RuleType::VariableDefinition) {
        auto mods = expectModifiers(ModifierTargetType::Variable);
        auto keyword = expect(TokenType::Identifier);
        if (!(keyword.valid && (keyword.token.raw == "let" || keyword.token.raw == "var"))) return nullptr;
        auto name = expect(TokenType::Identifier);
        if (!name.valid) return nullptr;
        if (!expect(TokenType::Colon).valid) return nullptr;
        auto type = expect(RuleType::Type);
        auto eq = expect(TokenType::EqualSign);
        std::shared_ptr<AST::ExpressionNode> initExpr = nullptr;
        if (eq.valid) {
          auto expr = expect(RuleType::Expression);
          if (!expr.valid) return nullptr;
          initExpr = std::dynamic_pointer_cast<AST::ExpressionNode>(expr.item);
        }
        auto varDef = std::make_shared<AST::VariableDefinitionExpression>(name.token.raw, std::dynamic_pointer_cast<AST::Type>(type.item), initExpr);
        varDef->modifiers = mods;
        return varDef;
      } else if (rule == RuleType::Fetch) {
        auto id = expect(TokenType::Identifier);
        if (!id.valid) return nullptr;
        return std::make_shared<AST::Fetch>(id.token.raw);
      } else if (rule == RuleType::Accessor) {
        rulesToIgnore.push_back(RuleType::Accessor);
        auto exprExp = expect(RuleType::Expression);
        rulesToIgnore.pop_back();
        if (!exprExp.valid) return nullptr;
        auto dot = expect(TokenType::Dot);
        if (!dot.valid) return nullptr;
        auto queryExp = expect(TokenType::Identifier);
        if (!queryExp.valid) return nullptr;
        auto acc = std::make_shared<AST::Accessor>(std::dynamic_pointer_cast<AST::ExpressionNode>(exprExp.item), queryExp.token.raw);
        while (true) {
          dot = expect(TokenType::Dot);
          if (!dot.valid) break;
          queryExp = expect(TokenType::Identifier);
          if (!queryExp.valid) break;
          auto tmp = acc;
          acc = std::make_shared<AST::Accessor>(acc, queryExp.token.raw);
        }
        return acc;
      } else if (rule == RuleType::Assignment) {
        rulesToIgnore.push_back(RuleType::Assignment);
        auto targetExp = expect(RuleType::Expression);
        rulesToIgnore.pop_back();
        if (!targetExp.valid) return nullptr;
        if (!expect(TokenType::EqualSign).valid) return nullptr;
        auto valueExp = expect(RuleType::Expression);
        if (!valueExp.valid) return nullptr;
        return std::make_shared<AST::AssignmentExpression>(std::dynamic_pointer_cast<AST::ExpressionNode>(targetExp.item), std::dynamic_pointer_cast<AST::ExpressionNode>(valueExp.item));
      }
      return nullptr;
    };
    void Parser::parse() {
      Expectation exp = expect(RuleType::Root);
      if (!exp.valid) return;
      root = std::dynamic_pointer_cast<AST::RootNode>(exp.item);
    };
  };
};
