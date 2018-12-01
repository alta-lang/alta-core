#include <algorithm>
#include "../include/altacore/parser.hpp"
#include "../include/altacore/util.hpp"

namespace AltaCore {
  namespace Parser {
    // <helper-functions>
    std::vector<std::shared_ptr<AST::Parameter>> Parser::expectParameters() {
      std::vector<std::shared_ptr<AST::Parameter>> parameters;
      Expectation param;
      param = expect(RuleType::Parameter);
      if (param.valid) {
        do {
          std::shared_ptr<AST::Parameter> parameter = std::dynamic_pointer_cast<AST::Parameter>(param.item.value());
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
    bool Parser::expectKeyword(std::string keyword) {
      auto state = currentState;
      auto exp = expect(TokenType::Identifier);
      if (exp && exp.token.raw == keyword) return true;
      currentState = state;
      return false;
    };
    // </helper-functions>

    std::optional<std::shared_ptr<AST::Node>> Parser::runRule(RuleType rule) {
      if (rule == RuleType::Root) {
        // TODO: use custom `ParserError`s instead of throwing `std::runtime_error`s
        std::vector<std::shared_ptr<AST::StatementNode>> statements;
        Expectation exp;
        while ((exp = expect({
          RuleType::ModuleOnlyStatement,
          RuleType::Statement,
        })), exp.valid) {
          auto stmt = std::dynamic_pointer_cast<AST::StatementNode>(exp.item.value());
          if (stmt == nullptr) throw std::runtime_error("AST node given was not of the expected type");
          statements.push_back(stmt);
        }
        if (currentState.currentPosition < tokens.size()) {
          throw std::runtime_error("Oof."); // TODO: better error message. i'm just lazy right now
        }
        return std::make_shared<AST::RootNode>(statements);
      } else if (rule == RuleType::Statement) {
        auto exp = expect({
          RuleType::GeneralAttribute,
          RuleType::FunctionDefinition,
          RuleType::FunctionDeclaration,
          RuleType::ReturnDirective,
          RuleType::Expression,
        });
        expect(TokenType::Semicolon); // optional
        if (!exp.valid) return std::nullopt;
        auto ret = std::dynamic_pointer_cast<AST::StatementNode>(exp.item.value());
        if (!exp.type.isToken && exp.type.rule == RuleType::Expression) {
          auto expr = std::dynamic_pointer_cast<AST::ExpressionNode>(exp.item.value());
          if (expr == nullptr) throw std::runtime_error("wtf");
          ret = std::make_shared<AST::ExpressionStatement>(expr);
        }
        return ret;
      } else if (rule == RuleType::Expression) {
        if (expect(TokenType::OpeningParenthesis)) {
          auto expr = expect(RuleType::Expression);
          if (!expr) return std::nullopt;
          if (!expect(TokenType::ClosingParenthesis)) return std::nullopt;
          return expr.item;
        } else {
          // lowest to highest precedence
          auto exp = expect({
            RuleType::VariableDefinition,
            RuleType::Assignment,
            RuleType::AdditionOrSubtraction,
            RuleType::MultiplicationOrDivision,
            RuleType::FunctionCall,

            // <special>
            RuleType::BooleanLiteral,
            RuleType::IntegralLiteral,
            RuleType::String,
            RuleType::Accessor,
            RuleType::Fetch,
            // </special>
          });
          return exp.item;
        }
      } else if (rule == RuleType::FunctionDefinition) {
        auto modifiers = expectModifiers(ModifierTargetType::Function);
        auto $function = expect(TokenType::Identifier);
        if (!($function.valid && $function.token.raw == "function")) return std::nullopt;
        auto name = expect(TokenType::Identifier);
        if (!name.valid) return std::nullopt;
        if (!expect(TokenType::OpeningParenthesis).valid) return std::nullopt;
        auto parameters = expectParameters();
        if (!expect(TokenType::ClosingParenthesis).valid) return std::nullopt;
        if (!expect(TokenType::Colon).valid) return std::nullopt;
        auto returnType = expect(RuleType::Type);
        if (!returnType.valid) return std::nullopt;
        if (!expect(TokenType::OpeningBrace).valid) return std::nullopt;
        std::vector<std::shared_ptr<AST::StatementNode>> statements;
        Expectation stmt;
        while ((stmt = expect(RuleType::Statement)), stmt.valid) {
          auto statement = std::dynamic_pointer_cast<AST::StatementNode>(stmt.item.value());
          if (statement == nullptr) throw std::runtime_error("uhm...");
          statements.push_back(statement);
        }
        if (!expect(TokenType::ClosingBrace).valid) return std::nullopt;
        return std::make_shared<AST::FunctionDefinitionNode>(name.token.raw, parameters, std::dynamic_pointer_cast<AST::Type>(returnType.item.value()), modifiers, std::make_shared<AST::BlockNode>(statements));
      } else if (rule == RuleType::Parameter) {
        auto name = expect(TokenType::Identifier);
        if (!name.valid) return std::nullopt;
        if (!expect(TokenType::Colon).valid) return std::nullopt;
        auto type = expect(RuleType::Type);
        if (!type.valid) return std::nullopt;
        auto actualType = std::dynamic_pointer_cast<AST::Type>(type.item.value());
        auto actualName = name.token.raw;
        return std::make_shared<AST::Parameter>(actualName, actualType);
      } else if (rule == RuleType::Type) {
        auto modifiers = expectModifiers(ModifierTargetType::Type);
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

        if (expect(TokenType::OpeningParenthesis)) {
          std::vector<std::shared_ptr<AST::Type>> args;
          auto firstType = expect(RuleType::Type);
          if (expect(TokenType::Comma)) {
            args.push_back(std::dynamic_pointer_cast<AST::Type>(firstType.item.value()));
            auto next = expect(RuleType::Type);
            while (next) {
              args.push_back(std::dynamic_pointer_cast<AST::Type>(next.item.value()));
              if (!expect(TokenType::Comma)) break;
              next = expect(RuleType::Type);
            }
            expect(TokenType::Comma); // optional trailing comma
          }
          if (!expect(TokenType::ClosingParenthesis)) return std::nullopt;
          if (expect(TokenType::Returns)) {
            if (args.size() < 1 && firstType) {
              args.push_back(std::dynamic_pointer_cast<AST::Type>(firstType.item.value()));
            }
            auto ret = expect(RuleType::Type);
            if (!ret) return std::nullopt;
            return std::make_shared<AST::Type>(std::dynamic_pointer_cast<AST::Type>(ret.item.value()), args, modifierBitflags);
          } else if (args.size() > 0) {
            // somehow, we detected parameters, but there's no return indicator,
            // so this isn't a type
            return std::nullopt;
          } else {
            if (!firstType) return std::nullopt;
            auto type = std::dynamic_pointer_cast<AST::Type>(firstType.item.value());
            type->modifiers.insert(type->modifiers.begin(), modifierBitflags.begin(), modifierBitflags.end());
            return type;
          }
        } else {
          auto name = expect(TokenType::Identifier);
          if (!name.valid) return std::nullopt;
          return std::make_shared<AST::Type>(name.token.raw, modifierBitflags);
        }
      } else if (rule == RuleType::IntegralLiteral) {
        auto integer = expect(TokenType::Integer);
        if (!integer.valid) return std::nullopt;
        return std::make_shared<AST::IntegerLiteralNode>(integer.token.raw);
      } else if (rule == RuleType::ReturnDirective) {
        auto keyword = expect(TokenType::Identifier);
        if (!(keyword.valid && keyword.token.raw == "return")) return std::nullopt;
        rulesToIgnore.push_back(RuleType::ReturnDirective);
        auto expr = expect(RuleType::Expression);
        rulesToIgnore.pop_back();
        std::shared_ptr<AST::ExpressionNode> exprNode = nullptr;
        if (expr.valid) {
          exprNode = std::dynamic_pointer_cast<AST::ExpressionNode>(expr.item.value());
        }
        return std::make_shared<AST::ReturnDirectiveNode>(exprNode);
      } else if (rule == RuleType::VariableDefinition) {
        auto mods = expectModifiers(ModifierTargetType::Variable);
        auto keyword = expect(TokenType::Identifier);
        if (!(keyword.valid && (keyword.token.raw == "let" || keyword.token.raw == "var"))) return std::nullopt;
        auto name = expect(TokenType::Identifier);
        if (!name.valid) return std::nullopt;
        if (!expect(TokenType::Colon).valid) return std::nullopt;
        auto type = expect(RuleType::Type);
        auto eq = expect(TokenType::EqualSign);
        std::shared_ptr<AST::ExpressionNode> initExpr = nullptr;
        if (eq.valid) {
          auto expr = expect(RuleType::Expression);
          if (!expr.valid) return std::nullopt;
          initExpr = std::dynamic_pointer_cast<AST::ExpressionNode>(expr.item.value());
        }
        auto varDef = std::make_shared<AST::VariableDefinitionExpression>(name.token.raw, std::dynamic_pointer_cast<AST::Type>(type.item.value()), initExpr);
        varDef->modifiers = mods;
        return varDef;
      } else if (rule == RuleType::Fetch) {
        auto id = expect(TokenType::Identifier);
        if (!id.valid) return std::nullopt;
        return std::make_shared<AST::Fetch>(id.token.raw);
      } else if (rule == RuleType::Accessor) {
        rulesToIgnore.push_back(RuleType::Accessor);
        auto exprExp = expect(RuleType::Expression);
        rulesToIgnore.pop_back();
        if (!exprExp.valid) return std::nullopt;
        auto dot = expect(TokenType::Dot);
        if (!dot.valid) return std::nullopt;
        auto queryExp = expect(TokenType::Identifier);
        if (!queryExp.valid) return std::nullopt;
        auto acc = std::make_shared<AST::Accessor>(std::dynamic_pointer_cast<AST::ExpressionNode>(exprExp.item.value()), queryExp.token.raw);
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
        if (!targetExp.valid) return std::nullopt;
        if (!expect(TokenType::EqualSign).valid) return std::nullopt;
        auto valueExp = expect(RuleType::Expression);
        if (!valueExp.valid) return std::nullopt;
        return std::make_shared<AST::AssignmentExpression>(std::dynamic_pointer_cast<AST::ExpressionNode>(targetExp.item.value()), std::dynamic_pointer_cast<AST::ExpressionNode>(valueExp.item.value()));
      } else if (rule == RuleType::AdditionOrSubtraction) {
        // TODO: move the MDAS (as in, PEMDAS) logic into a reusable function
        //       for now, the code here and in MultiplicationOrDivision has been copy-pasted
        //       and edited where necessary (and that's not very DRY)
        rulesToIgnore.push_back(RuleType::AdditionOrSubtraction);
        auto leftExp = expect(RuleType::Expression);
        rulesToIgnore.pop_back();
        if (!leftExp.valid) return std::nullopt;

        auto opExp = expect({
          TokenType::PlusSign,
          TokenType::MinusSign,
        });
        if (!opExp.valid) return std::nullopt;
        auto op = AST::OperatorType::Addition;
        if (opExp.token.type == TokenType::MinusSign) {
          op = AST::OperatorType::Subtraction;
        }

        rulesToIgnore.push_back(RuleType::AdditionOrSubtraction);
        auto rightExp = expect(RuleType::Expression);
        rulesToIgnore.pop_back();
        if (!rightExp.valid) return std::nullopt;

        auto binOp = std::make_shared<AST::BinaryOperation>(op, std::dynamic_pointer_cast<AST::ExpressionNode>(leftExp.item.value()), std::dynamic_pointer_cast<AST::ExpressionNode>(rightExp.item.value()));

        while ((opExp = expect({ TokenType::PlusSign, TokenType::MinusSign })), opExp.valid) {
          if (opExp.token.type == TokenType::PlusSign) {
            op = AST::OperatorType::Addition;
          } else {
            op = AST::OperatorType::Subtraction;
          }

          rulesToIgnore.push_back(RuleType::AdditionOrSubtraction);
          rightExp = expect(RuleType::Expression);
          rulesToIgnore.pop_back();
          if (!rightExp.valid) break;

          binOp = std::make_shared<AST::BinaryOperation>(op, std::dynamic_pointer_cast<AST::ExpressionNode>(binOp), std::dynamic_pointer_cast<AST::ExpressionNode>(rightExp.item.value()));
        }

        return binOp;
      } else if (rule == RuleType::MultiplicationOrDivision) {
        rulesToIgnore.push_back(RuleType::MultiplicationOrDivision);
        auto leftExp = expect(RuleType::Expression);
        rulesToIgnore.pop_back();
        if (!leftExp.valid) return std::nullopt;

        auto opExp = expect({
          TokenType::Asterisk,
          TokenType::ForwardSlash,
        });
        if (!opExp.valid) return std::nullopt;
        auto op = AST::OperatorType::Multiplication;
        if (opExp.token.type == TokenType::ForwardSlash) {
          op = AST::OperatorType::Division;
        }

        rulesToIgnore.push_back(RuleType::MultiplicationOrDivision);
        auto rightExp = expect(RuleType::Expression);
        rulesToIgnore.pop_back();
        if (!rightExp.valid) return std::nullopt;

        auto binOp = std::make_shared<AST::BinaryOperation>(op, std::dynamic_pointer_cast<AST::ExpressionNode>(leftExp.item.value()), std::dynamic_pointer_cast<AST::ExpressionNode>(rightExp.item.value()));

        while ((opExp = expect({ TokenType::Asterisk, TokenType::ForwardSlash })), opExp.valid) {
          if (opExp.token.type == TokenType::Asterisk) {
            op = AST::OperatorType::Multiplication;
          } else {
            op = AST::OperatorType::Division;
          }

          rulesToIgnore.push_back(RuleType::MultiplicationOrDivision);
          rightExp = expect(RuleType::Expression);
          rulesToIgnore.pop_back();
          if (!rightExp.valid) break;

          binOp = std::make_shared<AST::BinaryOperation>(op, std::dynamic_pointer_cast<AST::ExpressionNode>(binOp), std::dynamic_pointer_cast<AST::ExpressionNode>(rightExp.item.value()));
        }

        return binOp;
      } else if (rule == RuleType::ModuleOnlyStatement) {
        auto exp = expect({
          RuleType::Import,
        });
        expect(TokenType::Semicolon); // optional
        if (!exp.valid) return std::nullopt;
        return std::dynamic_pointer_cast<AST::StatementNode>(exp.item.value());
      } else if (rule == RuleType::Import) {
        if (!expectKeyword("import")) return std::nullopt;
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
          if (!expect(TokenType::ClosingBrace)) return std::nullopt;
          if (!expectKeyword("from")) return std::nullopt;
          auto mod = expect(TokenType::String);
          if (!mod) return std::nullopt;
          modName = mod.token.raw.substr(1, mod.token.raw.length() - 2);
        } else {
          if (auto mod = expect(TokenType::String)) {
            isAlias = true;
            modName = mod.token.raw.substr(1, mod.token.raw.length() - 2);
            if (!expectKeyword("as")) return std::nullopt;
            auto aliasExp = expect(TokenType::Identifier);
            if (!aliasExp) return std::nullopt;
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
            if (imports.size() == 0) return std::nullopt; // braced cherry-pick imports can have 0, but not freestyle cherry-pick imports
            expect(TokenType::Comma); // optional trailing comma
            if (!from) {
              // we probably already got it in the while loop, but just in case, check for it here
              if (!expectKeyword("from")) return std::nullopt;
            }
            auto module = expect(TokenType::String);
            if (!module) return std::nullopt;
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
      } else if (rule == RuleType::FunctionCall) {
        rulesToIgnore.push_back(RuleType::FunctionCall);
        auto target = expect(RuleType::Expression);
        rulesToIgnore.pop_back();
        if (!target) return std::nullopt;

        if (!expect(TokenType::OpeningParenthesis)) return std::nullopt;

        std::vector<std::shared_ptr<AST::ExpressionNode>> arguments;
        auto arg = expect(RuleType::Expression);
        while (arg) {
          arguments.push_back(std::dynamic_pointer_cast<AST::ExpressionNode>(arg.item.value()));
          if (!expect(TokenType::Comma)) break;
          arg = expect(RuleType::Expression);
        }
        expect(TokenType::Comma); // optional trailing comma

        if (!expect(TokenType::ClosingParenthesis)) return std::nullopt;
        
        return std::make_shared<AST::FunctionCallExpression>(std::dynamic_pointer_cast<AST::ExpressionNode>(target.item.value()), arguments);
      } else if (rule == RuleType::String) {
        auto raw = expect(TokenType::String);
        if (!raw) return std::nullopt;
        return std::make_shared<AST::StringLiteralNode>(Util::unescape(raw.token.raw.substr(1, raw.token.raw.length() - 2)));
      } else if (rule == RuleType::FunctionDeclaration) {
        if (!expectKeyword("declare")) return std::nullopt;
        auto modifiers = expectModifiers(ModifierTargetType::Function);
        if (!expectKeyword("function")) return std::nullopt;
        auto name = expect(TokenType::Identifier);
        if (!name.valid) return std::nullopt;
        if (!expect(TokenType::OpeningParenthesis).valid) return std::nullopt;
        auto parameters = expectParameters();
        if (!expect(TokenType::ClosingParenthesis).valid) return std::nullopt;
        if (!expect(TokenType::Colon).valid) return std::nullopt;
        auto returnType = expect(RuleType::Type);
        if (!returnType.valid) return std::nullopt;
        return std::make_shared<AST::FunctionDeclarationNode>(name.token.raw, parameters, std::dynamic_pointer_cast<AST::Type>(returnType.item.value()), modifiers);
      } else if (rule == RuleType::Attribute) {
        if (!expect(TokenType::AtSign)) return std::nullopt;

        std::vector<std::string> accessors;
        auto idExp = expect(TokenType::Identifier);
        while (idExp) {
          accessors.push_back(idExp.token.raw);
          if (!expect(TokenType::Dot)) break;
          idExp = expect(TokenType::Identifier);
        }
        if (accessors.size() == 0) return std::nullopt;

        std::vector<std::shared_ptr<AST::LiteralNode>> arguments;
        if (expect(TokenType::OpeningParenthesis)) {
          auto exp = expect(RuleType::AnyLiteral);
          while (exp) {
            arguments.push_back(std::dynamic_pointer_cast<AST::LiteralNode>(exp.item.value()));
            if (!expect(TokenType::Comma)) break;
            exp = expect(RuleType::AnyLiteral);
          }
          expect(TokenType::Comma); // optional trailing comma
          if (!expect(TokenType::ClosingParenthesis)) return std::nullopt;
        }

        return std::make_shared<AST::AttributeNode>(accessors, arguments);
      } else if (rule == RuleType::GeneralAttribute) {
        if (!expect(TokenType::AtSign)) return std::nullopt;
        auto attrExp = expect(RuleType::Attribute);
        if (!attrExp) return std::nullopt;
        return std::make_shared<AST::AttributeStatement>(std::dynamic_pointer_cast<AST::AttributeNode>(attrExp.item.value()));
      } else if (rule == RuleType::AnyLiteral) {
        return expect({
          RuleType::IntegralLiteral,
          RuleType::BooleanLiteral,
          RuleType::String,
        }).item;
      }
      return std::nullopt;
    };
    void Parser::parse() {
      Expectation exp = expect(RuleType::Root);
      if (!exp.valid) return;
      root = exp.item;
    };
  };
};
