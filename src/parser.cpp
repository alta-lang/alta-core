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
          std::shared_ptr<AST::Parameter> parameter = std::dynamic_pointer_cast<AST::Parameter>(*param.item);
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

    ALTACORE_OPTIONAL<std::shared_ptr<AST::Node>> Parser::runRule(RuleType rule) {
      if (rule == RuleType::Root) {
        // TODO: use custom `ParserError`s instead of throwing `std::runtime_error`s
        std::vector<std::shared_ptr<AST::StatementNode>> statements;
        Expectation exp;
        while ((exp = expect({
          RuleType::ModuleOnlyStatement,
          RuleType::Statement,
        })), exp.valid) {
          auto stmt = std::dynamic_pointer_cast<AST::StatementNode>(*exp.item);
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
          RuleType::FunctionDeclaration,
          RuleType::ReturnDirective,
          RuleType::Expression,

          // general attributes must come last because
          // they're supposed to be able to interpreted as part of
          // other statements that accept attributes if any such
          // statement is present
          RuleType::GeneralAttribute,
        });
        expect(TokenType::Semicolon); // optional
        if (!exp.valid) return ALTACORE_NULLOPT;
        auto ret = std::dynamic_pointer_cast<AST::StatementNode>(*exp.item);
        if (!exp.type.isToken && exp.type.rule == RuleType::Expression) {
          auto expr = std::dynamic_pointer_cast<AST::ExpressionNode>(*exp.item);
          if (expr == nullptr) throw std::runtime_error("wtf");
          ret = std::make_shared<AST::ExpressionStatement>(expr);
        }
        return ret;
      } else if (rule == RuleType::Expression) {
        if (expect(TokenType::OpeningParenthesis)) {
          auto expr = expect(RuleType::Expression);
          if (!expr) return ALTACORE_NULLOPT;
          if (!expect(TokenType::ClosingParenthesis)) return ALTACORE_NULLOPT;
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
        if (!($function.valid && $function.token.raw == "function")) return ALTACORE_NULLOPT;
        auto name = expect(TokenType::Identifier);
        if (!name.valid) return ALTACORE_NULLOPT;
        if (!expect(TokenType::OpeningParenthesis).valid) return ALTACORE_NULLOPT;
        auto parameters = expectParameters();
        if (!expect(TokenType::ClosingParenthesis).valid) return ALTACORE_NULLOPT;
        if (!expect(TokenType::Colon).valid) return ALTACORE_NULLOPT;
        auto returnType = expect(RuleType::Type);
        if (!returnType.valid) return ALTACORE_NULLOPT;
        if (!expect(TokenType::OpeningBrace).valid) return ALTACORE_NULLOPT;
        std::vector<std::shared_ptr<AST::StatementNode>> statements;
        Expectation stmt;
        while ((stmt = expect(RuleType::Statement)), stmt.valid) {
          auto statement = std::dynamic_pointer_cast<AST::StatementNode>(*stmt.item);
          if (statement == nullptr) throw std::runtime_error("uhm...");
          statements.push_back(statement);
        }
        if (!expect(TokenType::ClosingBrace).valid) return ALTACORE_NULLOPT;
        return std::make_shared<AST::FunctionDefinitionNode>(name.token.raw, parameters, std::dynamic_pointer_cast<AST::Type>(*returnType.item), modifiers, std::make_shared<AST::BlockNode>(statements));
      } else if (rule == RuleType::Parameter) {
        auto name = expect(TokenType::Identifier);
        if (!name.valid) return ALTACORE_NULLOPT;
        if (!expect(TokenType::Colon).valid) return ALTACORE_NULLOPT;
        auto type = expect(RuleType::Type);
        if (!type.valid) return ALTACORE_NULLOPT;
        auto actualType = std::dynamic_pointer_cast<AST::Type>(*type.item);
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
            args.push_back(std::dynamic_pointer_cast<AST::Type>(*firstType.item));
            auto next = expect(RuleType::Type);
            while (next) {
              args.push_back(std::dynamic_pointer_cast<AST::Type>(*next.item));
              if (!expect(TokenType::Comma)) break;
              next = expect(RuleType::Type);
            }
            expect(TokenType::Comma); // optional trailing comma
          }
          if (!expect(TokenType::ClosingParenthesis)) return ALTACORE_NULLOPT;
          if (expect(TokenType::Returns)) {
            if (args.size() < 1 && firstType) {
              args.push_back(std::dynamic_pointer_cast<AST::Type>(*firstType.item));
            }
            auto ret = expect(RuleType::Type);
            if (!ret) return ALTACORE_NULLOPT;
            return std::make_shared<AST::Type>(std::dynamic_pointer_cast<AST::Type>(*ret.item), args, modifierBitflags);
          } else if (args.size() > 0) {
            // somehow, we detected parameters, but there's no return indicator,
            // so this isn't a type
            return ALTACORE_NULLOPT;
          } else {
            if (!firstType) return ALTACORE_NULLOPT;
            auto type = std::dynamic_pointer_cast<AST::Type>(*firstType.item);
            type->modifiers.insert(type->modifiers.begin(), modifierBitflags.begin(), modifierBitflags.end());
            return type;
          }
        } else {
          auto name = expect(TokenType::Identifier);
          if (!name.valid) return ALTACORE_NULLOPT;
          return std::make_shared<AST::Type>(name.token.raw, modifierBitflags);
        }
      } else if (rule == RuleType::IntegralLiteral) {
        auto integer = expect(TokenType::Integer);
        if (!integer.valid) return ALTACORE_NULLOPT;
        return std::make_shared<AST::IntegerLiteralNode>(integer.token.raw);
      } else if (rule == RuleType::ReturnDirective) {
        auto keyword = expect(TokenType::Identifier);
        if (!(keyword.valid && keyword.token.raw == "return")) return ALTACORE_NULLOPT;
        rulesToIgnore.push_back(RuleType::ReturnDirective);
        auto expr = expect(RuleType::Expression);
        rulesToIgnore.pop_back();
        std::shared_ptr<AST::ExpressionNode> exprNode = nullptr;
        if (expr.valid) {
          exprNode = std::dynamic_pointer_cast<AST::ExpressionNode>(*expr.item);
        }
        return std::make_shared<AST::ReturnDirectiveNode>(exprNode);
      } else if (rule == RuleType::VariableDefinition) {
        auto mods = expectModifiers(ModifierTargetType::Variable);
        auto keyword = expect(TokenType::Identifier);
        if (!(keyword.valid && (keyword.token.raw == "let" || keyword.token.raw == "var"))) return ALTACORE_NULLOPT;
        auto name = expect(TokenType::Identifier);
        if (!name.valid) return ALTACORE_NULLOPT;
        if (!expect(TokenType::Colon).valid) return ALTACORE_NULLOPT;
        auto type = expect(RuleType::Type);
        auto eq = expect(TokenType::EqualSign);
        std::shared_ptr<AST::ExpressionNode> initExpr = nullptr;
        if (eq.valid) {
          auto expr = expect(RuleType::Expression);
          if (!expr.valid) return ALTACORE_NULLOPT;
          initExpr = std::dynamic_pointer_cast<AST::ExpressionNode>(*expr.item);
        }
        auto varDef = std::make_shared<AST::VariableDefinitionExpression>(name.token.raw, std::dynamic_pointer_cast<AST::Type>(*type.item), initExpr);
        varDef->modifiers = mods;
        return varDef;
      } else if (rule == RuleType::Fetch) {
        auto id = expect(TokenType::Identifier);
        if (!id.valid) return ALTACORE_NULLOPT;
        return std::make_shared<AST::Fetch>(id.token.raw);
      } else if (rule == RuleType::Accessor) {
        rulesToIgnore.push_back(RuleType::Accessor);
        auto exprExp = expect(RuleType::Expression);
        rulesToIgnore.pop_back();
        if (!exprExp.valid) return ALTACORE_NULLOPT;
        auto dot = expect(TokenType::Dot);
        if (!dot.valid) return ALTACORE_NULLOPT;
        auto queryExp = expect(TokenType::Identifier);
        if (!queryExp.valid) return ALTACORE_NULLOPT;
        auto acc = std::make_shared<AST::Accessor>(std::dynamic_pointer_cast<AST::ExpressionNode>(*exprExp.item), queryExp.token.raw);
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
        if (!targetExp.valid) return ALTACORE_NULLOPT;
        if (!expect(TokenType::EqualSign).valid) return ALTACORE_NULLOPT;
        auto valueExp = expect(RuleType::Expression);
        if (!valueExp.valid) return ALTACORE_NULLOPT;
        return std::make_shared<AST::AssignmentExpression>(std::dynamic_pointer_cast<AST::ExpressionNode>(*targetExp.item), std::dynamic_pointer_cast<AST::ExpressionNode>(*valueExp.item));
      } else if (rule == RuleType::AdditionOrSubtraction) {
        // TODO: move the MDAS (as in, PEMDAS) logic into a reusable function
        //       for now, the code here and in MultiplicationOrDivision has been copy-pasted
        //       and edited where necessary (and that's not very DRY)
        rulesToIgnore.push_back(RuleType::AdditionOrSubtraction);
        auto leftExp = expect(RuleType::Expression);
        rulesToIgnore.pop_back();
        if (!leftExp.valid) return ALTACORE_NULLOPT;

        auto opExp = expect({
          TokenType::PlusSign,
          TokenType::MinusSign,
        });
        if (!opExp.valid) return ALTACORE_NULLOPT;
        auto op = AST::OperatorType::Addition;
        if (opExp.token.type == TokenType::MinusSign) {
          op = AST::OperatorType::Subtraction;
        }

        rulesToIgnore.push_back(RuleType::AdditionOrSubtraction);
        auto rightExp = expect(RuleType::Expression);
        rulesToIgnore.pop_back();
        if (!rightExp.valid) return ALTACORE_NULLOPT;

        auto binOp = std::make_shared<AST::BinaryOperation>(op, std::dynamic_pointer_cast<AST::ExpressionNode>(*leftExp.item), std::dynamic_pointer_cast<AST::ExpressionNode>(*rightExp.item));

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

          binOp = std::make_shared<AST::BinaryOperation>(op, std::dynamic_pointer_cast<AST::ExpressionNode>(binOp), std::dynamic_pointer_cast<AST::ExpressionNode>(*rightExp.item));
        }

        return binOp;
      } else if (rule == RuleType::MultiplicationOrDivision) {
        rulesToIgnore.push_back(RuleType::MultiplicationOrDivision);
        auto leftExp = expect(RuleType::Expression);
        rulesToIgnore.pop_back();
        if (!leftExp.valid) return ALTACORE_NULLOPT;

        auto opExp = expect({
          TokenType::Asterisk,
          TokenType::ForwardSlash,
        });
        if (!opExp.valid) return ALTACORE_NULLOPT;
        auto op = AST::OperatorType::Multiplication;
        if (opExp.token.type == TokenType::ForwardSlash) {
          op = AST::OperatorType::Division;
        }

        rulesToIgnore.push_back(RuleType::MultiplicationOrDivision);
        auto rightExp = expect(RuleType::Expression);
        rulesToIgnore.pop_back();
        if (!rightExp.valid) return ALTACORE_NULLOPT;

        auto binOp = std::make_shared<AST::BinaryOperation>(op, std::dynamic_pointer_cast<AST::ExpressionNode>(*leftExp.item), std::dynamic_pointer_cast<AST::ExpressionNode>(*rightExp.item));

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

          binOp = std::make_shared<AST::BinaryOperation>(op, std::dynamic_pointer_cast<AST::ExpressionNode>(binOp), std::dynamic_pointer_cast<AST::ExpressionNode>(*rightExp.item));
        }

        return binOp;
      } else if (rule == RuleType::ModuleOnlyStatement) {
        auto exp = expect({
          RuleType::Import,
        });
        expect(TokenType::Semicolon); // optional
        if (!exp.valid) return ALTACORE_NULLOPT;
        return std::dynamic_pointer_cast<AST::StatementNode>(*exp.item);
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
      } else if (rule == RuleType::FunctionCall) {
        rulesToIgnore.push_back(RuleType::FunctionCall);
        auto target = expect(RuleType::Expression);
        rulesToIgnore.pop_back();
        if (!target) return ALTACORE_NULLOPT;

        if (!expect(TokenType::OpeningParenthesis)) return ALTACORE_NULLOPT;

        std::vector<std::pair<std::string, std::shared_ptr<AST::ExpressionNode>>> arguments;
        auto state = currentState;
        auto name = expect(TokenType::Identifier);
        if (name && !expect(TokenType::Colon)) {
          name = Expectation(); // constructs an invalid expectation
          currentState = state;
        }
        auto arg = expect(RuleType::Expression);
        while (arg) {
          arguments.push_back({
            (name) ? name.token.raw : "",
            std::dynamic_pointer_cast<AST::ExpressionNode>(*arg.item)
          });
          if (!expect(TokenType::Comma)) break;
          state = currentState;
          name = expect(TokenType::Identifier);
          if (name && !expect(TokenType::Colon)) {
            name = Expectation();
            currentState = state;
          }
          arg = expect(RuleType::Expression);
        }
        expect(TokenType::Comma); // optional trailing comma

        if (!expect(TokenType::ClosingParenthesis)) return ALTACORE_NULLOPT;
        
        return std::make_shared<AST::FunctionCallExpression>(std::dynamic_pointer_cast<AST::ExpressionNode>(*target.item), arguments);
      } else if (rule == RuleType::String) {
        auto raw = expect(TokenType::String);
        if (!raw) return ALTACORE_NULLOPT;
        return std::make_shared<AST::StringLiteralNode>(Util::unescape(raw.token.raw.substr(1, raw.token.raw.length() - 2)));
      } else if (rule == RuleType::FunctionDeclaration) {
        if (!expectKeyword("declare")) return ALTACORE_NULLOPT;
        auto modifiers = expectModifiers(ModifierTargetType::Function);
        if (!expectKeyword("function")) return ALTACORE_NULLOPT;
        auto name = expect(TokenType::Identifier);
        if (!name.valid) return ALTACORE_NULLOPT;
        if (!expect(TokenType::OpeningParenthesis).valid) return ALTACORE_NULLOPT;
        auto parameters = expectParameters();
        if (!expect(TokenType::ClosingParenthesis).valid) return ALTACORE_NULLOPT;
        if (!expect(TokenType::Colon).valid) return ALTACORE_NULLOPT;
        auto returnType = expect(RuleType::Type);
        if (!returnType.valid) return ALTACORE_NULLOPT;
        return std::make_shared<AST::FunctionDeclarationNode>(name.token.raw, parameters, std::dynamic_pointer_cast<AST::Type>(*returnType.item), modifiers);
      } else if (rule == RuleType::Attribute) {
        if (!expect(TokenType::AtSign)) return ALTACORE_NULLOPT;

        std::vector<std::string> accessors;
        auto idExp = expect(TokenType::Identifier);
        while (idExp) {
          accessors.push_back(idExp.token.raw);
          if (!expect(TokenType::Dot)) break;
          idExp = expect(TokenType::Identifier);
        }
        if (accessors.size() == 0) return ALTACORE_NULLOPT;

        std::vector<std::shared_ptr<AST::LiteralNode>> arguments;
        if (expect(TokenType::OpeningParenthesis)) {
          auto exp = expect(RuleType::AnyLiteral);
          while (exp) {
            arguments.push_back(std::dynamic_pointer_cast<AST::LiteralNode>(*exp.item));
            if (!expect(TokenType::Comma)) break;
            exp = expect(RuleType::AnyLiteral);
          }
          expect(TokenType::Comma); // optional trailing comma
          if (!expect(TokenType::ClosingParenthesis)) return ALTACORE_NULLOPT;
        }

        return std::make_shared<AST::AttributeNode>(accessors, arguments);
      } else if (rule == RuleType::GeneralAttribute) {
        if (!expect(TokenType::AtSign)) return ALTACORE_NULLOPT;
        auto attrExp = expect(RuleType::Attribute);
        if (!attrExp) return ALTACORE_NULLOPT;
        return std::make_shared<AST::AttributeStatement>(std::dynamic_pointer_cast<AST::AttributeNode>(*attrExp.item));
      } else if (rule == RuleType::AnyLiteral) {
        return expect({
          RuleType::IntegralLiteral,
          RuleType::BooleanLiteral,
          RuleType::String,
        }).item;
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
