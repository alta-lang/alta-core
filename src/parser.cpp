#include <algorithm>
#include "../include/altacore/parser.hpp"
#include "../include/altacore/util.hpp"

namespace AltaCore {
  namespace Parser {
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
          if (mod.token.raw == modifier) {
            return mod.token.raw;
          }
        }
      }
      currentState = state;
      return ALTACORE_NULLOPT;
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
    bool Parser::expectKeyword(std::string keyword) {
      auto state = currentState;
      auto exp = expect(TokenType::Identifier);
      if (exp && exp.token.raw == keyword) return true;
      currentState = state;
      return false;
    };
    Parser::RuleReturn Parser::expectBinaryOperation(RuleType rule, RuleType nextHigherPrecedentRule, std::vector<ExpectationType> operatorTokens, std::vector<AST::OperatorType> operatorTypes, RuleState& state, std::vector<Expectation>& exps) {
      if (operatorTokens.size() != operatorTypes.size()) {
        throw std::runtime_error("malformed binary operation expectation: the number of operator tokens must match the number of operator types.");
      }
      if (state.internalIndex == 0) {
        state.internalIndex = 1;
        return nextHigherPrecedentRule;
      } else if (state.internalIndex == 1) {
        if (!exps.back()) return ALTACORE_NULLOPT;

        auto binOp = std::make_shared<AST::BinaryOperation>();
        binOp->left = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);

        auto opExp = expect(operatorTokens);
        if (!opExp) return exps.back().item;

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

        return nextHigherPrecedentRule;
      } else {
        auto [savedState, binOp] = ALTACORE_ANY_CAST<std::pair<decltype(currentState), std::shared_ptr<AST::BinaryOperation>>>(state.internalValue);

        if (!exps.back()) {
          if (state.internalIndex == 2) {
            return ALTACORE_NULLOPT;
          } else {
            currentState = savedState;
            return binOp->left;
          }
        }

        binOp->right = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);

        Expectation opExp = expect(operatorTokens);

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
          return nextHigherPrecedentRule;
        } else {
          return binOp;
        }
      }
    };
    // </helper-functions>

    Parser::Parser(std::vector<Token> _tokens, Filesystem::Path _filePath):
      GenericParser(_tokens),
      filePath(_filePath)
      {};

    Parser::RuleReturn Parser::runRule(RuleType rule, RuleState& state, std::vector<Expectation>& exps) {
      auto result = realRunRule(rule, state, exps);
      if (auto node = ALTACORE_VARIANT_GET_IF<ALTACORE_OPTIONAL<NodeType>>(&result)) {
        if (*node) {
          auto& ptr = **node;
          ptr->position.line = tokens[state.stateAtStart.currentPosition].line;
          ptr->position.column = tokens[state.stateAtStart.currentPosition].column;
          ptr->position.file = filePath;
        }
      }
      return result;
    };

    Parser::RuleReturn Parser::realRunRule(RuleType rule, RuleState& state, std::vector<Expectation>& exps) {
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
        // TODO: use custom `ParserError`s instead of throwing `std::runtime_error`s

        std::initializer_list<ExpectationType> stmtType = {
          RuleType::ModuleOnlyStatement,
          RuleType::Statement,
        };

        // logic for the initial call
        if (state.iteration == 0) {
          return std::move(stmtType);
        }

        // basically a while loop that continues as long statements are available
        if (exps.back()) {
          return std::move(stmtType);
        }

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
        auto root = std::make_shared<AST::RootNode>(statements);
        return root;
      } else if (rule == RuleType::Statement) {
        if (state.iteration == 0) {
          return std::initializer_list<ExpectationType> {
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
            RuleType::GeneralAttribute,
          };
        }

        if (!exps[0]) return ALTACORE_NULLOPT;

        while (expect(TokenType::Semicolon)); // optional

        auto& exp = exps[0];
        auto ret = std::dynamic_pointer_cast<AST::StatementNode>(*exp.item);
        if (!exp.type.isToken && exp.type.rule == RuleType::Expression) {
          auto expr = std::dynamic_pointer_cast<AST::ExpressionNode>(*exp.item);
          if (expr == nullptr) throw std::runtime_error("wtf");
          ret = std::make_shared<AST::ExpressionStatement>(expr);
        }
        // doing this here means we've already got all the statements
        // covered; we don't have to repeat it for each statement
        return ret;
      } else if (rule == RuleType::Expression) {
        if (state.internalIndex == 0) {
          state.internalIndex = 1;
          return RuleType::VariableDefinition;
        } else {
          if (!exps.back()) return ALTACORE_NULLOPT;
          auto expr = *exps.back().item;
          // same thing about the statements here,
          // but here it covers all expressions
          return expr;
        }
        /*
        if (state.iteration == 0) {
          if (auto exp = expect(TokenType::OpeningParenthesis)) {
            exps.push_back(exp);
            return RuleType::Expression;
          } else {
            // precendence here is least to most
            // i.e. VariableDefinition has the least precedence,
            //      FunctionCallOrSubscript has the most precedence

            state.internalValue = currentState;

            return std::initializer_list<ExpectationType> {
              RuleType::VariableDefinition,

              // <front-recursive-rules>
              RuleType::Assignment,
              RuleType::VerbalConditionalExpression,
              RuleType::PunctualConditonalExpression,
              RuleType::EqualityRelationalOperation,
              RuleType::NonequalityRelationalOperation,
              RuleType::AdditionOrSubtraction,
              RuleType::MultiplicationOrDivision,
              RuleType::FunctionCallOrSubscript,
              // </front-recursive-rules>

              // <special>
              RuleType::BooleanLiteral,
              RuleType::IntegralLiteral,
              RuleType::String,
              RuleType::Accessor,
              RuleType::Fetch,
              // </special>
            };
          }
        }

        if (exps.front().type.isToken) {
          // continuation of wrapped expression check above
          if (!exps.back()) return ALTACORE_NULLOPT;
          if (!expect(TokenType::ClosingParenthesis)) return ALTACORE_NULLOPT;
          return exps.back().item;
        }

        return exps.back().item;
        */
      } else if (rule == RuleType::FunctionDefinition) {
        if (state.internalIndex == 0) {
          state.internalIndex = 1;
          return RuleType::Attribute;
        } else if (state.internalIndex == 1) {
          if (exps.back()) {
            return RuleType::Attribute;
          }

          exps.pop_back();
          
          auto funcDef = std::make_shared<AST::FunctionDefinitionNode>();
          funcDef->modifiers = expectModifiers(ModifierTargetType::Function);

          state.internalValue = std::move(funcDef);
          state.internalIndex = 2;
          return RuleType::Attribute;
        } else if (state.internalIndex == 2) {
          if (exps.back()) {
            return RuleType::Attribute;
          }

          exps.pop_back();

          auto funcDef = ALTACORE_ANY_CAST<std::shared_ptr<AST::FunctionDefinitionNode>>(state.internalValue);

          for (auto& exp: exps) {
            funcDef->attributes.push_back(std::dynamic_pointer_cast<AST::AttributeNode>(*exp.item));
          }
          exps.clear();

          if (!expectKeyword("function")) return ALTACORE_NULLOPT;

          auto name = expect(TokenType::Identifier);
          if (!name) return ALTACORE_NULLOPT;
          funcDef->name = name.token.raw;

          if (!expect(TokenType::OpeningParenthesis)) return ALTACORE_NULLOPT;

          state.internalIndex = 3;
          return RuleType::Parameter;
        } else if (state.internalIndex == 3) {
          auto funcDef = ALTACORE_ANY_CAST<std::shared_ptr<AST::FunctionDefinitionNode>>(state.internalValue);

          if (exps.back()) {
            std::shared_ptr<AST::Parameter> parameter = std::dynamic_pointer_cast<AST::Parameter>(*exps.back().item);
            if (parameter == nullptr) throw std::runtime_error("oh no.");
            funcDef->parameters.push_back(parameter);

            exps.pop_back();

            if (expect(TokenType::Comma)) {
              return RuleType::Parameter;
            }
          }

          exps.clear(); // we don't need those parameter expecatations anymore

          if (!expect(TokenType::ClosingParenthesis)) return ALTACORE_NULLOPT;
          if (!expect(TokenType::Colon)) return ALTACORE_NULLOPT;

          state.internalIndex = 4;
          return RuleType::Type;
        } else if (state.internalIndex == 4) {
          auto funcDef = ALTACORE_ANY_CAST<std::shared_ptr<AST::FunctionDefinitionNode>>(state.internalValue);

          if (!exps.back()) return ALTACORE_NULLOPT;
          funcDef->returnType = std::dynamic_pointer_cast<AST::Type>(*exps.back().item);

          state.internalIndex = 5;
          return RuleType::Block;
        } else {
          auto funcDef = ALTACORE_ANY_CAST<std::shared_ptr<AST::FunctionDefinitionNode>>(state.internalValue);

          if (!exps.back()) return ALTACORE_NULLOPT;
          funcDef->body = std::dynamic_pointer_cast<AST::BlockNode>(*exps.back().item);

          return std::move(funcDef);
        }
      } else if (rule == RuleType::Parameter) {
        if (state.internalIndex == 0) {
          state.internalIndex = 1;
          return RuleType::Attribute;
        } else if (state.internalIndex == 1) {
          if (exps.back()) {
            return RuleType::Attribute;
          }

          exps.pop_back(); // remove the last (implicitly invalid) expectation

          auto param = std::make_shared<AST::Parameter>();

          for (auto& exp: exps) {
            param->attributes.push_back(std::dynamic_pointer_cast<AST::AttributeNode>(*exp.item));
          }

          auto name = expect(TokenType::Identifier);
          if (!name) return ALTACORE_NULLOPT;
          param->name = name.token.raw;

          if (!expect(TokenType::Colon)) return ALTACORE_NULLOPT;

          state.internalValue = std::move(param);
          state.internalIndex = 2;

          typesToIgnore.insert("any");
          return RuleType::Type;
        } else {
          typesToIgnore.erase("any");

          bool isAny = false;
          if (!exps.back()) {
            if (expectKeyword("any")) {
              isAny = true;
            } else {
              return ALTACORE_NULLOPT;
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

          return std::move(param);
        }
      } else if (rule == RuleType::StrictAccessor) {
        if (state.internalIndex == 0) {
          state.internalIndex = 1;
          return std::initializer_list<ExpectationType> {
            RuleType::Fetch,
          };
        } else {
          if (!exps.back()) return ALTACORE_NULLOPT;

          if (!expect(TokenType::Dot)) return exps.back().item;

          auto query = expect(TokenType::Identifier);
          if (!query) return ALTACORE_NULLOPT;

          auto acc = std::make_shared<AST::Accessor>(std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item), query.token.raw);

          auto state = currentState;
          while (expect(TokenType::Dot)) {
            query = expect(TokenType::Identifier);
            if (!query) {
              currentState = state;
              break;
            }
            acc = std::make_shared<AST::Accessor>(acc, query.token.raw);
            state = currentState;
          }

          return acc;
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
            return RuleType::StrictAccessor;
          }

          state.internalIndex = 1;

          return RuleType::Type;
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

              return RuleType::Type;
            }
          }

          exps.pop_back();

          if (!expect(TokenType::ClosingParenthesis)) return ALTACORE_NULLOPT;

          if (expect(TokenType::Returns)) {
            // if we continue, we're parsing a function pointer type
            type->isFunction = true;

            state.internalIndex = 2;
            return RuleType::Type;
          } else if (type->parameters.size() > 1) {
            // somehow, we detected parameters, but there's no return indicator,
            // so this isn't a type
            return ALTACORE_NULLOPT;
          } else {
            auto otherType = std::get<0>(type->parameters[0]);
            otherType->modifiers.insert(otherType->modifiers.begin(), type->modifiers.begin(), type->modifiers.end());
            return otherType;
          }
        } else if (state.internalIndex == 2) {
          if (!exps.back()) return ALTACORE_NULLOPT;

          auto type = ALTACORE_ANY_CAST<std::shared_ptr<AST::Type>>(state.internalValue);

          type->returnType = std::dynamic_pointer_cast<AST::Type>(*exps.back().item);
          return type;
        } else {
          auto type = ALTACORE_ANY_CAST<std::shared_ptr<AST::Type>>(state.internalValue);

          if (!exps.back()) return ALTACORE_NULLOPT;

          auto expr = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);

          if (expr->nodeType() == AST::NodeType::Fetch) {
            auto id = std::dynamic_pointer_cast<AST::Fetch>(expr);
            auto name = id->query;
            if (name == "int" || name == "byte" || name == "char" || name == "bool" || name == "void") {
              if (typesToIgnore.find(name) != typesToIgnore.end()) return ALTACORE_NULLOPT;
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

          return type;
        }
      } else if (rule == RuleType::IntegralLiteral) {
        auto integer = expect(TokenType::Integer);
        if (!integer) return ALTACORE_NULLOPT;
        return std::make_shared<AST::IntegerLiteralNode>(integer.token.raw);
      } else if (rule == RuleType::ReturnDirective) {
        if (state.internalIndex == 0) {
          if (!expectKeyword("return")) return ALTACORE_NULLOPT;
          state.internalIndex = 1;
          return RuleType::Expression;
        } else {
          std::shared_ptr<AST::ExpressionNode> expr = nullptr;
          if (exps.back()) {
            expr = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);
          }
          return std::make_shared<AST::ReturnDirectiveNode>(expr);
        }
      } else if (rule == RuleType::Block) {
        if (state.internalIndex == 0) {
          if (!expect(TokenType::OpeningBrace)) return ALTACORE_NULLOPT;

          state.internalIndex = 1;
          state.internalValue = std::make_shared<AST::BlockNode>();

          return RuleType::Statement;
        } else {
          auto block = ALTACORE_ANY_CAST<std::shared_ptr<AST::BlockNode>>(state.internalValue);

          if (exps.back()) {
            block->statements.push_back(std::dynamic_pointer_cast<AST::StatementNode>(*exps.back().item));

            exps.pop_back();
            return RuleType::Statement;
          }

          if (!expect(TokenType::ClosingBrace)) return ALTACORE_NULLOPT;
          return block;
        }
      } else if (rule == RuleType::VariableDefinition) {
        if (state.internalIndex == 0) {
          auto varDef = std::make_shared<AST::VariableDefinitionExpression>();

          const auto saved = currentState;
          varDef->modifiers = expectModifiers(ModifierTargetType::Variable);

          if (!expectKeyword("let") && !expectKeyword("var")) {
            currentState = saved;
            state.internalIndex = 3;
            return RuleType::Assignment;
          }

          auto name = expect(TokenType::Identifier);
          if (!name) return ALTACORE_NULLOPT;
          varDef->name = name.token.raw;

          state.internalValue = std::move(varDef);

          if (expect(TokenType::EqualSign)) {
            state.internalIndex = 2;
            return RuleType::Expression;
          } else if (!expect(TokenType::Colon)) {
            return ALTACORE_NULLOPT;
          }

          state.internalIndex = 1;
          return RuleType::Type;
        } else if (state.internalIndex == 1) {
          if (!exps.back()) return ALTACORE_NULLOPT;
          auto varDef = ALTACORE_ANY_CAST<std::shared_ptr<AST::VariableDefinitionExpression>>(state.internalValue);
          varDef->type = std::dynamic_pointer_cast<AST::Type>(*exps.back().item);

          if (expect(TokenType::EqualSign)) {
            state.internalIndex = 2;
            return RuleType::Expression;
          } else {
            return varDef;
          }
        } else if (state.internalIndex == 3) {
          return exps.back().item;
        } else {
          // we're expecting a value to initialize the variable
          if (!exps.back()) return ALTACORE_NULLOPT;
          auto varDef = ALTACORE_ANY_CAST<std::shared_ptr<AST::VariableDefinitionExpression>>(state.internalValue);
          varDef->initializationExpression = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);

          return varDef;
        }
      } else if (rule == RuleType::Fetch) {
        auto id = expect(TokenType::Identifier);
        if (!id) return ALTACORE_NULLOPT;
        return std::make_shared<AST::Fetch>(id.token.raw);
      } else if (rule == RuleType::Accessor) {
        if (state.internalIndex == 0) {
          state.internalIndex = 1;
          if (inClass) {
            return std::initializer_list<ExpectationType> {
              RuleType::SuperClassFetch,
              RuleType::Fetch,
              RuleType::GroupedExpression,
            };
          } else {
            return std::initializer_list<ExpectationType> {
              RuleType::Fetch,
              RuleType::GroupedExpression,
            };
          }
        } else {
          if (!exps.back()) return ALTACORE_NULLOPT;

          if (!expect(TokenType::Dot)) return exps.back().item;

          auto query = expect(TokenType::Identifier);
          if (!query) return ALTACORE_NULLOPT;

          auto acc = std::make_shared<AST::Accessor>(std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item), query.token.raw);

          auto state = currentState;
          while (expect(TokenType::Dot)) {
            query = expect(TokenType::Identifier);
            if (!query) {
              currentState = state;
              break;
            }
            acc = std::make_shared<AST::Accessor>(acc, query.token.raw);
            state = currentState;
          }

          return acc;
        }
      } else if (rule == RuleType::Assignment) {
        if (state.internalIndex == 0) {
          state.internalIndex = 1;
          return RuleType::VerbalConditionalExpression;
        } else if (state.internalIndex == 1) {
          if (!exps.back()) return ALTACORE_NULLOPT;

          if (!expect(TokenType::EqualSign)) return exps.back().item;

          state.internalValue = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);
          state.internalIndex = 2;

          return RuleType::Assignment;
        } else {
          if (!exps.back()) return ALTACORE_NULLOPT;

          auto lhs = ALTACORE_ANY_CAST<std::shared_ptr<AST::ExpressionNode>>(state.internalValue);

          return std::make_shared<AST::AssignmentExpression>(lhs, std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item));
        }
      } else if (rule == RuleType::AdditionOrSubtraction) {
        return expectBinaryOperation(rule, RuleType::MultiplicationOrDivision, {
          TokenType::PlusSign,
          TokenType::MinusSign,
        }, {
          AST::OperatorType::Addition,
          AST::OperatorType::Subtraction,
        }, state, exps);
      } else if (rule == RuleType::MultiplicationOrDivision) {
        return expectBinaryOperation(rule, RuleType::Cast, {
          TokenType::Asterisk,
          TokenType::ForwardSlash,
        }, {
          AST::OperatorType::Multiplication,
          AST::OperatorType::Division,
        }, state, exps);
      } else if (rule == RuleType::ModuleOnlyStatement) {
        if (state.iteration == 0) {
          return std::initializer_list<ExpectationType> {
            RuleType::Import,
          };
        } else {
          while (expect(TokenType::Semicolon)); // optional
          if (!exps.back()) return ALTACORE_NULLOPT;
          return exps.back().item;
        }
      } else if (rule == RuleType::Import) {
        if (!expectKeyword("import")) return ALTACORE_NULLOPT;
        bool isAlias = false;
        std::string modName;
        std::vector<std::pair<std::string, std::string>> imports;
        std::string alias;
        if (expect(TokenType::OpeningBrace)) {
          Expectation importExp = expect(TokenType::Identifier);
          while (importExp) {
            std::string aliasString = "";
            if (expectKeyword("as")) {
              auto aliasExp = expect(TokenType::Identifier);
              if (!aliasExp) break;
              aliasString = aliasExp.token.raw;
            }
            imports.push_back({ importExp.token.raw, aliasString });
            if (!expect(TokenType::Comma)) break;
            importExp = expect(TokenType::Identifier);
          }
          expect(TokenType::Comma); // optional trailing comma
          if (!expect(TokenType::ClosingBrace)) return ALTACORE_NULLOPT;
          if (!expectKeyword("from")) return ALTACORE_NULLOPT;
          auto mod = expect(TokenType::String);
          if (!mod) return ALTACORE_NULLOPT;
          modName = mod.token.raw.substr(1, mod.token.raw.length() - 2);
        } else {
          if (auto mod = expect(TokenType::String)) {
            isAlias = true;
            modName = mod.token.raw.substr(1, mod.token.raw.length() - 2);
            if (!expectKeyword("as")) return ALTACORE_NULLOPT;
            auto aliasExp = expect(TokenType::Identifier);
            if (!aliasExp) return ALTACORE_NULLOPT;
            alias = aliasExp.token.raw;
          } else {
            Expectation importExp = expect(TokenType::Identifier);
            bool from = false;
            while (importExp) {
              if (importExp.token.raw == "from") {
                from = true;
                break;
              }
              std::string aliasString = "";
              if (expectKeyword("as")) {
                auto aliasExp = expect(TokenType::Identifier);
                if (!aliasExp) break;
                aliasString = aliasExp.token.raw;
              }
              imports.push_back({ importExp.token.raw, aliasString });
              if (!expect(TokenType::Comma)) break;
              importExp = expect(TokenType::Identifier);
            }
            if (imports.size() == 0) return ALTACORE_NULLOPT; // braced cherry-pick imports can have 0, but not freestyle cherry-pick imports
            expect(TokenType::Comma); // optional trailing comma
            if (!from) {
              // we probably already got it in the while loop, but just in case, check for it here
              if (!expectKeyword("from")) return ALTACORE_NULLOPT;
            }
            auto module = expect(TokenType::String);
            if (!module) return ALTACORE_NULLOPT;
            modName = module.token.raw.substr(1, module.token.raw.length() - 2);
          }
        }
        modName = Util::unescape(modName);
        if (isAlias) {
          return std::make_shared<AST::ImportStatement>(modName, alias);
        } else {
          return std::make_shared<AST::ImportStatement>(modName, imports);
        }
      } else if (rule == RuleType::BooleanLiteral) {
        if (expectKeyword("true")) {
          return std::make_shared<AST::BooleanLiteralNode>(true);
        } else if (expectKeyword("false")) {
          return std::make_shared<AST::BooleanLiteralNode>(false);
        }
      } else if (rule == RuleType::FunctionCallOrSubscript) {
        if (state.internalIndex == 0) {
          state.internalIndex = 1;
          return std::initializer_list<ExpectationType> {
            RuleType::ClassInstantiation,
          };
        } else if (state.internalIndex == 1) {
          if (!exps.back()) return ALTACORE_NULLOPT;

          bool isSubscript = false;

          if (expect(TokenType::OpeningSquareBracket)) isSubscript = true;
          if (!isSubscript && !expect(TokenType::OpeningParenthesis)) return exps.back().item;

          if (!isSubscript) {
            auto funcCall = std::make_shared<AST::FunctionCallExpression>();
            funcCall->target = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);

            auto tmpState = currentState;
            auto name = expect(TokenType::Identifier);
            if (name && !expect(TokenType::Colon)) {
              name = Expectation(); // constructs an invalid expectation
              currentState = tmpState;
            }
            funcCall->arguments.push_back({
              (name) ? name.token.raw : "",
              nullptr,
            });

            state.internalValue = std::move(funcCall);
            state.internalIndex = 2;

            return RuleType::Expression;
          } else {
            auto subs = std::make_shared<AST::SubscriptExpression>();
            subs->target = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);

            state.internalValue = std::move(subs);
            state.internalIndex = 3;
            
            return RuleType::Expression;
          }
        } else if (state.internalIndex == 2) {
          auto callState = ALTACORE_ANY_CAST<std::shared_ptr<AST::FunctionCallExpression>>(state.internalValue);

          if (exps.back()) {
            callState->arguments.back().second = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);
            if (expect(TokenType::Comma)) {
              auto tmpState = currentState;
              auto name = expect(TokenType::Identifier);
              if (name && !expect(TokenType::Colon)) {
                name = Expectation(); // constructs an invalid expectation
                currentState = tmpState;
              }

              callState->arguments.push_back({
                (name) ? name.token.raw : "",
                nullptr,
              });

              return RuleType::Expression;
            }
          } else {
            callState->arguments.pop_back();
          }

          if (!expect(TokenType::ClosingParenthesis)) return ALTACORE_NULLOPT;

          if (expect(TokenType::OpeningParenthesis)) {
            auto newCall = std::make_shared<AST::FunctionCallExpression>(callState);

            auto tmpState = currentState;
            auto name = expect(TokenType::Identifier);
            if (name && !expect(TokenType::Colon)) {
              name = Expectation(); // constructs an invalid expectation
              currentState = tmpState;
            }
            newCall->arguments.push_back({
              (name) ? name.token.raw : "",
              nullptr,
            });
            state.internalValue = std::move(newCall);
            return RuleType::Expression;
          } else if (expect(TokenType::OpeningSquareBracket)) {
            auto subs = std::make_shared<AST::SubscriptExpression>();
            subs->target = callState;
            
            state.internalValue = std::move(subs);
            state.internalIndex = 3;

            return RuleType::Expression;
          }

          return callState;
        } else if (state.internalIndex == 3) {
          auto subs = ALTACORE_ANY_CAST<std::shared_ptr<AST::SubscriptExpression>>(state.internalValue);

          if (!exps.back()) return ALTACORE_NULLOPT; // TODO: error recovery

          if (!expect(TokenType::ClosingSquareBracket)) return ALTACORE_NULLOPT;

          subs->index = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);

          if (expect(TokenType::OpeningParenthesis)) {
            auto call = std::make_shared<AST::FunctionCallExpression>(subs);

            auto tmpState = currentState;
            auto name = expect(TokenType::Identifier);
            if (name && !expect(TokenType::Colon)) {
              name = Expectation(); // constructs an invalid expectation
              currentState = tmpState;
            }
            call->arguments.push_back({
              (name) ? name.token.raw : "",
              nullptr,
            });
            state.internalValue = std::move(call);
            state.internalIndex = 2;

            return RuleType::Expression;
          } else if (expect(TokenType::OpeningSquareBracket)) {
            auto newSubs = std::make_shared<AST::SubscriptExpression>();
            newSubs->target = subs;
            
            state.internalValue = std::move(newSubs);

            return RuleType::Expression;
          }

          return subs;
        }
      } else if (rule == RuleType::String) {
        auto raw = expect(TokenType::String);
        if (!raw) return ALTACORE_NULLOPT;
        return std::make_shared<AST::StringLiteralNode>(Util::unescape(raw.token.raw.substr(1, raw.token.raw.length() - 2)));
      } else if (rule == RuleType::Character) {
        auto raw = expect(TokenType::Character);
        if (!raw) return ALTACORE_NULLOPT;
        auto cont = raw.token.raw.substr(1, raw.token.raw.length() - 2);
        return std::make_shared<AST::CharacterLiteralNode>((cont.length() == 2) ? cont[1] : cont[0], cont.length() == 2);
      } else if (rule == RuleType::FunctionDeclaration) {
        if (state.internalIndex == 0) {
          if (!expectKeyword("declare")) return ALTACORE_NULLOPT;

          auto funcDecl = std::make_shared<AST::FunctionDeclarationNode>();
          funcDecl->modifiers = expectModifiers(ModifierTargetType::Function);

          if (!expectKeyword("function")) return ALTACORE_NULLOPT;

          auto name = expect(TokenType::Identifier);
          if (!name) return ALTACORE_NULLOPT;
          funcDecl->name = name.token.raw;

          if (!expect(TokenType::OpeningParenthesis)) return ALTACORE_NULLOPT;

          state.internalValue = std::move(funcDecl);
          state.internalIndex = 1;
          return RuleType::Parameter;
        } else if (state.internalIndex == 1) {
          auto funcDecl = ALTACORE_ANY_CAST<std::shared_ptr<AST::FunctionDeclarationNode>>(state.internalValue);

          if (exps.back()) {
            std::shared_ptr<AST::Parameter> parameter = std::dynamic_pointer_cast<AST::Parameter>(*exps.back().item);
            if (parameter == nullptr) throw std::runtime_error("oh no.");
            funcDecl->parameters.push_back(parameter);

            exps.pop_back();

            if (expect(TokenType::Comma)) {
              return RuleType::Parameter;
            }
          }

          exps.clear(); // we don't need those parameter expecatations anymore

          if (!expect(TokenType::ClosingParenthesis)) return ALTACORE_NULLOPT;
          if (!expect(TokenType::Colon)) return ALTACORE_NULLOPT;

          state.internalIndex = 2;
          return RuleType::Type;
        } else {
          auto funcDecl = ALTACORE_ANY_CAST<std::shared_ptr<AST::FunctionDeclarationNode>>(state.internalValue);

          if (!exps.back()) return ALTACORE_NULLOPT;
          funcDecl->returnType = std::dynamic_pointer_cast<AST::Type>(*exps.back().item);

          return std::move(funcDecl);
        }
      } else if (rule == RuleType::Attribute) {
        if (state.internalIndex == 0) {
          if (!expect(TokenType::AtSign)) return ALTACORE_NULLOPT;

          auto attr = std::make_shared<AST::AttributeNode>();

          auto idExp = expect(TokenType::Identifier);
          while (idExp) {
            attr->accessors.push_back(idExp.token.raw);
            if (!expect(TokenType::Dot)) break;
            idExp = expect(TokenType::Identifier);
          }
          if (attr->accessors.size() == 0) return ALTACORE_NULLOPT;

          if (expect(TokenType::OpeningParenthesis)) {
            state.internalIndex = 1;
            state.internalValue = std::move(attr);

            return RuleType::AnyLiteral;
          } else {
            return std::move(attr);
          }
        } else {
          auto attr = ALTACORE_ANY_CAST<std::shared_ptr<AST::AttributeNode>>(state.internalValue);

          if (exps.back()) {
            attr->arguments.push_back(std::dynamic_pointer_cast<AST::LiteralNode>(*exps.back().item));

            if (expect(TokenType::Comma)) {
              return RuleType::AnyLiteral;
            }
          }

          if (!expect(TokenType::ClosingParenthesis)) return ALTACORE_NULLOPT;

          return attr;
        }
      } else if (rule == RuleType::GeneralAttribute) {
        if (state.internalIndex == 0) {
          if (!expect(TokenType::AtSign)) return ALTACORE_NULLOPT;
          state.internalIndex = 1;
          return RuleType::Attribute;
        } else {
          if (!exps.back()) return ALTACORE_NULLOPT;
          return std::make_shared<AST::AttributeStatement>(std::dynamic_pointer_cast<AST::AttributeNode>(*exps.back().item));
        }
      } else if (rule == RuleType::AnyLiteral) {
        if (state.internalIndex == 0) {
          state.internalIndex = 1;
          return std::initializer_list<ExpectationType> {
            RuleType::IntegralLiteral,
            RuleType::BooleanLiteral,
            RuleType::String,
            RuleType::Character,
          };
        } else {
          return exps.back().item;
        }
      } else if (rule == RuleType::ConditionalStatement) {
        if (state.internalIndex == 0) {
          if (!expectKeyword("if")) return ALTACORE_NULLOPT;

          state.internalIndex = 1;

          return RuleType::Expression;
        } else if (state.internalIndex == 1) {
          if (!exps.back()) return ALTACORE_NULLOPT;

          auto cond = std::make_shared<AST::ConditionalStatement>();
          cond->primaryTest = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);

          state.internalValue = ConditionStatementState(currentState, std::move(cond));
          state.internalIndex = 2;

          return RuleType::Statement;
        } else if (state.internalIndex == 2) {
          if (!exps.back()) return ALTACORE_NULLOPT;

          auto intern = ALTACORE_ANY_CAST<ConditionStatementState<decltype(currentState)>>(state.internalValue);

          intern.cond->primaryResult = std::dynamic_pointer_cast<AST::StatementNode>(*exps.back().item);

          intern.state = currentState;
          if (expectKeyword("else")) {
            if (expectKeyword("if")) {
              state.internalIndex = 3;
              return RuleType::Expression;
            } else {
              state.internalIndex = 5;
              return RuleType::Statement;
            }
          }

          return intern.cond;
        } else if (state.internalIndex == 3) {
          auto intern = ALTACORE_ANY_CAST<ConditionStatementState<decltype(currentState)>>(state.internalValue);

          if (!exps.back()) {
            currentState = intern.state;
            return intern.cond;
          }

          intern.cond->alternatives.push_back({
            std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item),
            nullptr,
          });

          state.internalIndex = 4;

          return RuleType::Statement;
        } else if (state.internalIndex == 4) {
          auto intern = ALTACORE_ANY_CAST<ConditionStatementState<decltype(currentState)>>(state.internalValue);

          if (!exps.back()) {
            currentState = intern.state;
            intern.cond->alternatives.pop_back();
            return intern.cond;
          }

          intern.cond->alternatives.back().second = std::dynamic_pointer_cast<AST::StatementNode>(*exps.back().item);

          state.internalValue = ConditionStatementState(currentState, intern.cond);
          if (expectKeyword("else")) {
            if (expectKeyword("if")) {
              state.internalIndex = 3;
              return RuleType::Expression;
            } else {
              state.internalIndex = 5;
              return RuleType::Statement;
            }
          }

          return intern.cond;
        } else if (state.internalIndex == 5) {
          auto intern = ALTACORE_ANY_CAST<ConditionStatementState<decltype(currentState)>>(state.internalValue);

          if (!exps.back()) {
            currentState = intern.state;
            return intern.cond;
          }

          intern.cond->finalResult = std::dynamic_pointer_cast<AST::StatementNode>(*exps.back().item);

          return intern.cond;
        }
      } else if (rule == RuleType::VerbalConditionalExpression) {
        if (state.internalIndex == 0) {
          state.internalIndex = 1;
          return RuleType::PunctualConditonalExpression;
        } else if (state.internalIndex == 1) {
          if (!exps.back()) return ALTACORE_NULLOPT;

          State stateCache = currentState;
          if (!expectKeyword("if")) return exps.back().item;

          auto cond = std::make_shared<AST::ConditionalExpression>();
          cond->primaryResult = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);

          state.internalValue = VerbalConditionalState<State>(std::move(cond), stateCache);
          state.internalIndex = 2;

          return RuleType::Expression;
        } else if (state.internalIndex == 2) {
          auto ruleState = ALTACORE_ANY_CAST<VerbalConditionalState<State>>(state.internalValue);

          if (!exps.back()) {
            currentState = ruleState.stateCache;
            return ruleState.cond->primaryResult;
          }

          if (!expectKeyword("else")) {
            currentState = ruleState.stateCache;
            return ruleState.cond->primaryResult;
          }

          ruleState.cond->test = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);

          state.internalIndex = 3;
          return RuleType::PunctualConditonalExpression;
        } else if (state.internalIndex == 3) {
          auto ruleState = ALTACORE_ANY_CAST<VerbalConditionalState<State>>(state.internalValue);

          if (!exps.back()) {
            currentState = ruleState.stateCache;
            return ruleState.cond->primaryResult;
          }

          ruleState.cond->secondaryResult = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);

          State stateCache = currentState;
          if (expectKeyword("if")) {
            auto newCond = std::make_shared<AST::ConditionalExpression>();
            newCond->primaryResult = ruleState.cond;

            state.internalValue = VerbalConditionalState<State>(std::move(newCond), stateCache, true);
            state.internalIndex = 2;

            return RuleType::Expression;
          }

          return ruleState.cond;
        }
      } else if (rule == RuleType::PunctualConditonalExpression) {
        if (state.internalIndex == 0) {
          state.internalIndex = 1;
          return RuleType::EqualityRelationalOperation;
        } else if (state.internalIndex == 1) {
          if (!exps.back()) return ALTACORE_NULLOPT;

          if (!expect(TokenType::QuestionMark)) return exps.back().item;

          auto cond = std::make_shared<AST::ConditionalExpression>();
          cond->test = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);

          state.internalIndex = 2;
          state.internalValue = std::move(cond);

          return RuleType::Expression;
        } else if (state.internalIndex == 2) {
          if (!exps.back()) return ALTACORE_NULLOPT;

          if (!expect(TokenType::Colon)) return ALTACORE_NULLOPT;

          auto cond = ALTACORE_ANY_CAST<std::shared_ptr<AST::ConditionalExpression>>(state.internalValue);
          cond->primaryResult = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);

          state.internalIndex = 3;

          return RuleType::PunctualConditonalExpression;
        } else if (state.internalIndex == 3) {
          if (!exps.back()) return ALTACORE_NULLOPT;

          auto cond = ALTACORE_ANY_CAST<std::shared_ptr<AST::ConditionalExpression>>(state.internalValue);
          cond->secondaryResult = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);

          return cond;
        }
      } else if (rule == RuleType::NonequalityRelationalOperation) {
        return expectBinaryOperation(rule, RuleType::Instanceof, {
          TokenType::OpeningAngleBracket,
          TokenType::ClosingAngleBracket,
          TokenType::LessThanOrEqualTo,
          TokenType::GreaterThanOrEqualTo,
        }, {
          AST::OperatorType::LessThan,
          AST::OperatorType::GreaterThan,
          AST::OperatorType::LessThanOrEqualTo,
          AST::OperatorType::GreaterThanOrEqualTo,
        }, state, exps);
      } else if (rule == RuleType::EqualityRelationalOperation) {
        return expectBinaryOperation(rule, RuleType::NonequalityRelationalOperation, {
          TokenType::Equality,
          TokenType::Inequality,
        }, {
          AST::OperatorType::EqualTo,
          AST::OperatorType::NotEqualTo,
        }, state, exps);
      } else if (rule == RuleType::GroupedExpression) {
        if (state.internalIndex == 0) {
          if (!expect(TokenType::OpeningParenthesis)) return ALTACORE_NULLOPT;
          state.internalIndex = 1;
          return RuleType::Expression;
        } else {
          if (!exps.back()) return ALTACORE_NULLOPT;
          if (!expect(TokenType::ClosingParenthesis)) return ALTACORE_NULLOPT;
          return exps.back().item;
        }
      } else if (rule == RuleType::ClassDefinition) {
        if (state.internalIndex == 0) {
          auto mods = expectModifiers(ModifierTargetType::Class);
          if (!expectKeyword("class")) return ALTACORE_NULLOPT;

          auto id = expect(TokenType::Identifier);
          if (!id) return ALTACORE_NULLOPT;

          auto def = std::make_shared<AST::ClassDefinitionNode>(id.token.raw);
          def->modifiers = mods;

          if (expectKeyword("extends")) {
            bool dot = false;
            while (auto pid = expect(TokenType::Identifier)) {
              if (dot) {
                auto& back = def->parents.back();
                back = std::make_shared<AST::Accessor>(back, pid.token.raw);
              } else {
                def->parents.push_back(std::make_shared<AST::Fetch>(pid.token.raw));
              }
              if (expect(TokenType::Dot)) {
                dot = true;
              } else if (!expect(TokenType::Comma)) {
                break;
              }
            }
            if (dot) return ALTACORE_NULLOPT;
          }

          if (!expect(TokenType::OpeningBrace)) return ALTACORE_NULLOPT;

          state.internalValue = std::move(def);
          state.internalIndex = 1;
          return RuleType::ClassStatement;
        } else {
          auto klass = ALTACORE_ANY_CAST<std::shared_ptr<AST::ClassDefinitionNode>>(state.internalValue);

          if (exps.back()) {
            klass->statements.push_back(std::dynamic_pointer_cast<AST::ClassStatementNode>(*exps.back().item));
            return RuleType::ClassStatement;
          }

          if (!expect(TokenType::ClosingBrace)) return ALTACORE_NULLOPT;

          return klass;
        }
      } else if (rule == RuleType::ClassStatement) {
        if (state.iteration == 0) {
          return std::initializer_list<ExpectationType> {
            RuleType::ClassMember,
            RuleType::ClassSpecialMethod,
            RuleType::ClassMethod,
          };
        }

        if (!exps.back()) return ALTACORE_NULLOPT;

        while (expect(TokenType::Semicolon)); // optional

        return exps.back().item;
      } else if (rule == RuleType::ClassMember) {
        if (state.internalIndex == 0) {
          auto visibilityMod = expectModifier(ModifierTargetType::ClassStatement);
          if (!visibilityMod) return ALTACORE_NULLOPT;

          state.internalValue = std::make_shared<AST::ClassMemberDefinitionStatement>(AST::parseVisibility(*visibilityMod));
          state.internalIndex = 1;
          return RuleType::VariableDefinition;
        } else {
          if (!exps.back()) return ALTACORE_NULLOPT;

          auto memberDef = ALTACORE_ANY_CAST<std::shared_ptr<AST::ClassMemberDefinitionStatement>>(state.internalValue);
          memberDef->varDef = std::dynamic_pointer_cast<AST::VariableDefinitionExpression>(*exps.back().item);
          if (!memberDef->varDef) return ALTACORE_NULLOPT;

          return memberDef;
        }
      } else if (rule == RuleType::ClassMethod) {
        if (state.internalIndex == 0) {
          state.internalIndex = 1;
          return RuleType::Attribute;
        } else if (state.internalIndex == 1) {
          if (exps.back()) {
            return RuleType::Attribute;
          }

          exps.pop_back();

          auto visibilityMod = expectModifier(ModifierTargetType::ClassStatement);
          if (!visibilityMod) return ALTACORE_NULLOPT;

          std::vector<std::shared_ptr<AST::AttributeNode>> attrs;

          for (auto& exp: exps) {
            attrs.push_back(std::dynamic_pointer_cast<AST::AttributeNode>(*exp.item));
          }

          state.internalValue = std::make_pair(attrs, std::make_shared<AST::ClassMethodDefinitionStatement>(AST::parseVisibility(*visibilityMod)));
          state.internalIndex = 2;
          inClass = true;
          return RuleType::FunctionDefinition;
        } else {
          inClass = false;
          if (!exps.back()) return ALTACORE_NULLOPT;

          auto [attrs, methodDef] = ALTACORE_ANY_CAST<std::pair<std::vector<std::shared_ptr<AST::AttributeNode>>, std::shared_ptr<AST::ClassMethodDefinitionStatement>>>(state.internalValue);
          methodDef->funcDef = std::dynamic_pointer_cast<AST::FunctionDefinitionNode>(*exps.back().item);
          if (!methodDef->funcDef) return ALTACORE_NULLOPT;

          methodDef->funcDef->attributes.insert(methodDef->funcDef->attributes.begin(), attrs.begin(), attrs.end());

          return methodDef;
        }
      } else if (rule == RuleType::ClassSpecialMethod) {
        if (state.internalIndex == 0) {
          state.internalIndex = 1;
          return RuleType::Attribute;
        } else if (state.internalIndex == 1) {
          if (exps.back()) {
            return RuleType::Attribute;
          } else {
            exps.pop_back();
          }

          auto visibilityMod = expectModifier(ModifierTargetType::ClassStatement);
          if (!visibilityMod) return ALTACORE_NULLOPT;

          auto method = std::make_shared<AST::ClassSpecialMethodDefinitionStatement>(AST::parseVisibility(*visibilityMod), AST::SpecialClassMethod::Constructor);

          state.internalValue = std::move(method);
          state.internalIndex = 2;

          return RuleType::Attribute;
        } else if (state.internalIndex == 2) {
          if (exps.back()) {
            return RuleType::Attribute;
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
            return ALTACORE_NULLOPT;
          }

          method->type = kind;

          if (!expect(TokenType::OpeningParenthesis)) return ALTACORE_NULLOPT;
          
          state.internalIndex = 3;
          return RuleType::Parameter;
        } else if (state.internalIndex == 3) {
          auto method = ALTACORE_ANY_CAST<std::shared_ptr<AST::ClassSpecialMethodDefinitionStatement>>(state.internalValue);

          if (exps.back()) {
            method->parameters.push_back(std::dynamic_pointer_cast<AST::Parameter>(*exps.back().item));
            if (expect(TokenType::Comma)) {
              return RuleType::Parameter;
            }
          }

          if (!expect(TokenType::ClosingParenthesis)) return ALTACORE_NULLOPT;

          state.internalIndex = 4;
          inClass = true;
          return RuleType::Block;
        } else {
          inClass = false;
          if (!exps.back()) return ALTACORE_NULLOPT;

          auto method = ALTACORE_ANY_CAST<std::shared_ptr<AST::ClassSpecialMethodDefinitionStatement>>(state.internalValue);
          method->body = std::dynamic_pointer_cast<AST::BlockNode>(*exps.back().item);

          return method;
        }
      } else if (rule == RuleType::ClassInstantiation) {
        if (state.internalIndex == 0) {
          state.internalValue = currentState;
          if (!expectKeyword("new")) {
            state.internalIndex = 4;
            if (inClass) {
              state.internalIndex = 5;
              return std::initializer_list<ExpectationType> {
                RuleType::SuperClassFetch,
              };
            }
            return std::initializer_list<ExpectationType> {
              RuleType::BooleanLiteral,
              RuleType::IntegralLiteral,
              RuleType::String,
              RuleType::Character,
              RuleType::Accessor,
            };
          }
          state.internalIndex = 1;
          return RuleType::Accessor;
        } else if (state.internalIndex == 1 || state.internalIndex == 5) {
          if (!exps.back()) {
            if (state.internalIndex != 5) {
              currentState = ALTACORE_ANY_CAST<decltype(currentState)>(state.internalValue);
            }
            state.internalIndex = 4;
            return std::initializer_list<ExpectationType> {
              RuleType::BooleanLiteral,
              RuleType::IntegralLiteral,
              RuleType::String,
              RuleType::Character,
              RuleType::Accessor,
            };
          }

          auto inst = std::make_shared<AST::ClassInstantiationExpression>();

          inst->target = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);
          bool isSuperclassFetch = inst->target->nodeType() == AST::NodeType::SuperClassFetch;

          if (expect(TokenType::OpeningParenthesis)) {
            auto tmpState = currentState;
            auto name = expect(TokenType::Identifier);
            if (name && !expect(TokenType::Colon)) {
              name = Expectation(); // constructs an invalid expectation
              currentState = tmpState;
            }
            inst->arguments.push_back({
              (name) ? name.token.raw : "",
              nullptr,
            });

            state.internalIndex = 2;
            state.internalValue = std::move(inst);
            return RuleType::Expression;
          } else if (isSuperclassFetch) {
            currentState = ALTACORE_ANY_CAST<decltype(currentState)>(state.internalValue);
            state.internalIndex = 4;
            return std::initializer_list<ExpectationType> {
              RuleType::BooleanLiteral,
              RuleType::IntegralLiteral,
              RuleType::String,
              RuleType::Character,
              RuleType::Accessor,
            };
          }

          return inst;
        } else if (state.internalIndex == 2) {
          auto inst = ALTACORE_ANY_CAST<std::shared_ptr<AST::ClassInstantiationExpression>>(state.internalValue);

          if (exps.back()) {
            inst->arguments.back().second = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);
            if (expect(TokenType::Comma)) {
              auto tmpState = currentState;
              auto name = expect(TokenType::Identifier);
              if (name && !expect(TokenType::Colon)) {
                name = Expectation(); // constructs an invalid expectation
                currentState = tmpState;
              }

              inst->arguments.push_back({
                (name) ? name.token.raw : "",
                nullptr,
              });

              return RuleType::Expression;
            }
          } else {
            inst->arguments.pop_back();
          }

          if (!expect(TokenType::ClosingParenthesis)) return ALTACORE_NULLOPT;

          return inst;
        } else if (state.internalIndex == 4) {
          return exps.back().item;
        }
      } else if (rule == RuleType::Cast) {
        if (state.internalIndex == 0) {
          state.internalIndex = 1;
          return RuleType::PointerOrDereference;
        } else if (state.internalIndex == 1) {
          if (!exps.back()) return ALTACORE_NULLOPT;
          auto cast = std::make_shared<AST::CastExpression>();
          cast->target = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);
          state.internalValue = std::make_pair(std::move(cast), currentState);
          state.internalIndex = 2;
          if (!expectKeyword("as")) return exps.back().item;
          return RuleType::Type;
        } else {
          auto cast = ALTACORE_ANY_CAST<std::pair<std::shared_ptr<AST::CastExpression>, decltype(currentState)>>(state.internalValue);
          if (!exps.back()) {
            currentState = cast.second;
            return cast.first->target;
          }
          cast.first->type = std::dynamic_pointer_cast<AST::Type>(*exps.back().item);
          return cast.first;
        }
      } else if (rule == RuleType::PointerOrDereference) {
        if (state.internalIndex == 0) {
          if (expect(TokenType::Asterisk) || expectKeyword("valueof")) {
            state.internalIndex = 2;
          } else if (expect(TokenType::Ampersand) || expectKeyword("getptr")) {
            state.internalIndex = 3;
          } else {
            state.internalIndex = 1;
            return RuleType::FunctionCallOrSubscript;
          }

          return RuleType::PointerOrDereference;
        } else if (state.internalIndex == 1) {
          return exps.back().item;
        } else {
          if (!exps.back()) return ALTACORE_NULLOPT;

          auto expr = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);

          if (state.internalIndex == 2) {
            auto val = std::make_shared<AST::DereferenceExpression>();
            val->target = expr;
            return val;
          } else if (state.internalIndex == 3) {
            auto val = std::make_shared<AST::PointerExpression>();
            val->target = expr;
            return val;
          } else {
            return ALTACORE_NULLOPT;
          }
        }
      } else if (rule == RuleType::WhileLoop) {
        if (state.internalIndex == 0) {
          if (!expectKeyword("while")) return ALTACORE_NULLOPT;

          state.internalIndex = 1;
          return RuleType::Expression;
        } else if (state.internalIndex == 1) {
          if (!exps.back()) return ALTACORE_NULLOPT;

          auto loop = std::make_shared<AST::WhileLoopStatement>();
          loop->test = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);
          
          state.internalValue = std::move(loop);
          state.internalIndex = 2;
          return RuleType::Statement;
        } else {
          if (!exps.back()) return ALTACORE_NULLOPT;

          auto loop = ALTACORE_ANY_CAST<std::shared_ptr<AST::WhileLoopStatement>>(state.internalValue);
          loop->body = std::dynamic_pointer_cast<AST::StatementNode>(*exps.back().item);

          return loop;
        }
      } else if (rule == RuleType::TypeAlias) {
        if (state.internalIndex == 0) {
          auto mods = expectModifiers(ModifierTargetType::TypeAlias);
          if (!expectKeyword("type")) return ALTACORE_NULLOPT;
          auto name = expect(TokenType::Identifier);
          if (!name) return ALTACORE_NULLOPT;
          if (!expect(TokenType::EqualSign)) return ALTACORE_NULLOPT;
          auto typeAlias = std::make_shared<AST::TypeAliasStatement>();
          typeAlias->modifiers = mods;
          typeAlias->name = name.token.raw;
          state.internalValue = std::move(typeAlias);
          state.internalIndex = 1;
          return RuleType::Type;
        } else {
          if (!exps.back()) return ALTACORE_NULLOPT;
          auto typeAlias = ALTACORE_ANY_CAST<std::shared_ptr<AST::TypeAliasStatement>>(state.internalValue);
          typeAlias->type = std::dynamic_pointer_cast<AST::Type>(*exps.back().item);
          return typeAlias;
        }
      } else if (rule == RuleType::SuperClassFetch) {
        if (state.internalIndex == 0) {
          if (!expectKeyword("super")) return ALTACORE_NULLOPT;
          auto sup = std::make_shared<AST::SuperClassFetch>();
          if (expect(TokenType::OpeningAngleBracket)) {
            if (auto lit = expect(TokenType::Integer)) {
              sup->fetch = std::make_shared<AST::IntegerLiteralNode>(lit.token.raw);
              if (!expect(TokenType::ClosingAngleBracket)) return ALTACORE_NULLOPT;
            } else {
              state.internalIndex = 1;
              state.internalValue = std::move(sup);
              return RuleType::StrictAccessor;
            }
          }

          return sup;
        } else {
          auto sup = ALTACORE_ANY_CAST<std::shared_ptr<AST::SuperClassFetch>>(state.internalValue);

          if (!exps.back()) return ALTACORE_NULLOPT;
          sup->fetch = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);
          if (!expect(TokenType::ClosingAngleBracket)) return ALTACORE_NULLOPT;

          return sup;
        }
      } else if (rule == RuleType::Instanceof) {
        if (state.internalIndex == 0) {
          state.internalIndex = 1;
          return RuleType::AdditionOrSubtraction;
        } else if (state.internalIndex == 1) {
          if (!exps.back()) return ALTACORE_NULLOPT;
          if (!expectKeyword("instanceof")) return exps.back().item;
          auto instOf = std::make_shared<AST::InstanceofExpression>();
          instOf->target = std::dynamic_pointer_cast<AST::ExpressionNode>(*exps.back().item);
          state.internalValue = std::move(instOf);
          state.internalIndex = 2;
          return RuleType::Type;
        } else {
          auto instOf = ALTACORE_ANY_CAST<std::shared_ptr<AST::InstanceofExpression>>(state.internalValue);
          if (!exps.back()) return ALTACORE_NULLOPT;
          instOf->type = std::dynamic_pointer_cast<AST::Type>(*exps.back().item);
          return instOf;
        }
      }

      return ALTACORE_NULLOPT;
    };

    void Parser::parse() {
      Expectation exp = expect(RuleType::Root);
      if (!exp.valid) return;
      root = exp.item;
    };
  };
};
