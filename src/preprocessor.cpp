#include "../include/altacore/preprocessor.hpp"
#include "../include/altacore/modules.hpp"
#include "../include/altacore/lexer.hpp"
#include "../include/altacore/parser.hpp"
#include "../include/altacore/util.hpp"
#include <locale>
#include <vector>
#include <utility>
#include <fstream>

// i *highly* doubt that this preprocessor is efficient, but it works.
// plus, so far in the tests i've done, the performance hit is negligible.
// besides, Alta does AOT compiliation, so it shouldn't matter that much.

namespace PreprocessorUtils {
  std::string trim(std::string input) {
    auto defaultLocale = std::locale("");
    size_t firstNonWhitespace = std::string::npos;
    size_t lastNonWhitespace = std::string::npos;
    for (size_t i = 0; i < input.length(); i++) {
      if (!std::isspace(input[i], defaultLocale)) {
        firstNonWhitespace = i;
        break;
      }
    }
    for (size_t i = input.length(); i > 0; i++) {
      if (!std::isspace(input[i - 1], defaultLocale)) {
        lastNonWhitespace = i;
        break;
      }
    }
    if (firstNonWhitespace == std::string::npos || lastNonWhitespace == std::string::npos) {
      return "";
    }
    return input.substr(firstNonWhitespace, lastNonWhitespace - firstNonWhitespace + 1);
  };
  std::vector<std::string> splitByWhitespace(std::string input, bool dontTouchStrings = false) {
    auto defaultLocale = std::locale("");
    std::vector<std::string> results = { "" };

    bool inString = false;
    for (size_t i = 0; i < input.size(); i++) {
      auto& character = input[i];
      if (dontTouchStrings && character == '"') {
        inString = !inString;
        results[results.size() - 1] += character;
      } else if (dontTouchStrings && character == '\\' && inString && i + 1 < input.size()) {
        results[results.size() - 1] += character + input[i + 1];
        i++;
      } else if (inString || !std::isspace(character, defaultLocale)) {
        results[results.size() - 1] += character;
      } else {
        results.emplace_back();
      }
    }

    if (results.size() == 1 && results[0] == "") {
      results.clear();
    }

    return results;
  };
  std::vector<std::string> split(std::string input, char delimiter) {
    std::vector<std::string> results = { "" };

    for (auto& character: input) {
      if (character != delimiter) {
        results[results.size() - 1] += character;
      } else {
        results.emplace_back();
      }
    }

    if (results.size() == 1 && results[0] == "") {
      results.clear();
    }

    return results;
  };
  std::vector<std::vector<std::string>> splitLinesByWhitespace(std::string input, bool dontTouchStrings = false, bool trimLines = false) {
    auto lines = split(input, '\n');
    std::vector<std::vector<std::string>> results;
    for (auto& line: lines) {
      if (trimLines) {
        line = trim(line);
      }
      results.push_back(splitByWhitespace(line, dontTouchStrings));
    }
    return results;
  };
  std::vector<std::pair<size_t, size_t>> findStrings(std::string data) {
    if (data.find('"') == std::string::npos) return {};
    std::vector<std::pair<size_t, size_t>> stringLocations;
    bool inString = false;
    for (size_t i = 0; i < data.length(); i++) {
      if (data[i] == '"') {
        if (inString) {
          stringLocations[stringLocations.size() - 1].second = i;
        } else {
          stringLocations.emplace_back(i, i);
        }
        inString = !inString;
      }
    }
    return stringLocations;
  };
  std::vector<std::pair<size_t, size_t>> findSubstitutionDelimiters(std::string data) {
    std::vector<std::pair<size_t, size_t>> results;
    auto first = data.find("@[");
    if (first == std::string::npos) return {};
    auto second = data.find("]", first + 2);
    if (second == std::string::npos) return {};
    auto stringLocations = findStrings(data);
    while (first != std::string::npos && second != std::string::npos) {
      if (data.substr(first + 2, second - first - 1).find(' ') != std::string::npos) {
        // if there's a space, ignore the first one and search for a new first one
        // (reset the second too, just in case)
        first = data.find("@[", first + 2);
        second = data.find(']', first + 2);
      } else {
        // check whether either is contained in a string
        bool cont = false;
        for (auto& [stringStart, stringEnd]: stringLocations) {
          if (stringStart < second && stringEnd > second) {
            // if the second delimiter is contained in a string,
            // we can ignore them both
            first = data.find("@[", second + 1);
            second = data.find("]", first + 2);
            cont = true;
          }
          if (stringStart < first && stringEnd > first) {
            // if the first delimiter is contained in a string,
            // ignore it and try searching for a new second one
            //
            // btw, here we've already determined that the second delimiter
            // is not in a string
            first = data.find("@[", first + 2);
            second = data.find(']', first + 2);
            cont = true;
          }
          if (cont) {
            break;
          }
        }
        if (cont) {
          continue;
        }
        // if both of the delimiter are ok, add them to the results
        results.emplace_back(first, second);
        first = data.find("@[", second + 1);
        second = data.find(']', first + 1);
      }
    }
    return results;
  };
  bool isIdentifier(std::string data) {
    bool isFirst = true;
    for (auto& character: data) {
      // if the character is not one of the allowed characters, the given string is not an identifier
      if (!(
        (!isFirst && character > 0x2f && character < 0x3a) || // 0-9
        (character > 0x40 && character < 0x5b) ||             // A-Z
        (character > 0x60 && character < 0x7b) ||             // a-z
        character == '_'                                      // _
      )) {
        return false;
      }
      if (isFirst) {
        isFirst = false;
      }
    }
    return true;
  };
  class Index {
    private:
      std::vector<std::string>& tokens;
    public:
      size_t i;
      std::string& operator *() {
        return tokens[i];
      };
      std::string* operator ->() {
        return &tokens[i];
      };
      bool hasNext(size_t skipCount = 1) {
        return tokens.size() > i + skipCount;
      };
      Index& next(size_t skipCount = 1) {
        i += skipCount;
        return *this;
      };
      Index& back(size_t skipCount = 1)  {
        i -= skipCount;
        return *this;
      };
      std::string& peek(size_t skipCount = 1) {
        return tokens[i + skipCount];
      };
      Index(std::vector<std::string>& _tokens, size_t start = 0):
        tokens(_tokens),
        i(start)
        {};
  };
  std::string join(std::vector<std::string> data, std::string separator) {
    std::string result;

    bool isFirst = true;
    for (auto& item: data) {
      if (isFirst) {
        isFirst = false;
      } else {
        result += separator;
      }
      result += item;
    }

    return result;
  };
};

bool AltaCore::Preprocessor::Expression::operator ==(const AltaCore::Preprocessor::Expression& right) {
  if (type != right.type) return false;
  if (type == ExpressionType::Boolean) {
    return boolean == right.boolean;
  } else if (type == ExpressionType::String) {
    return string == right.string;
  } else {
    // both are null or undefined
    return true;
  }
};

AltaCore::Preprocessor::Expression::operator bool() {
  if (type == ExpressionType::Boolean) {
    return boolean;
  } else if (type == ExpressionType::String) {
    return !string.empty();
  } else {
    // null and undefined are non-truthy
    return false;
  }
};

AltaCore::Preprocessor::Expression AltaCore::Preprocessor::ExpressionParser::defined(std::vector<AltaCore::Preprocessor::Expression> targets) {
  for (auto& target: targets) {
    if (target.type == ExpressionType::Undefined) {
      return false;
    }
  }
  return true;
};

AltaCore::Preprocessor::ExpressionParser::ExpressionParser(std::vector<Lexer::Token> _tokens, std::map<std::string, Expression>& _definitions):
  GenericParser(_tokens),
  definitions(_definitions)
{
  /*
  internalRuleTable = {
    { RuleType::Expression, [&](Parser::RuleState& state, std::vector<Expectation>& exps) -> RuleReturn {
      if (state.iteration == 0) {
        return std::initializer_list<ExpectationType> {
          RuleType::Wrapped,
          RuleType::Or,
          RuleType::And,
          RuleType::Equality,
          RuleType::MacroCall,
          RuleType::BooleanLiteral,
          RuleType::Retrieval,
          RuleType::String,
        };
      }
      if (!exps[0]) return ALTACORE_NULLOPT;
      return exps[0].item;
    }},
    { RuleType::Equality, [&](Parser::RuleState& state, std::vector<Expectation>& exps) -> RuleReturn {
      if (state.internalIndex == 0) {
        rulesToIgnore.push_back(RuleType::Equality);
        state.internalIndex = 1;
        return RuleType::Expression;
      } else if (state.internalIndex == 1) {
        rulesToIgnore.pop_back();

        if (!exps.back()) return ALTACORE_NULLOPT;

        if (!expect(Lexer::TokenType::Equality)) return ALTACORE_NULLOPT;

        rulesToIgnore.push_back(RuleType::Equality);
        state.internalIndex = 2;
        return RuleType::Expression;
      } else {
        rulesToIgnore.pop_back();

        if (!exps.back()) return ALTACORE_NULLOPT;

        auto& left = (exps.size() == 1) ? std::any_cast<Expression>(state.internalValue) : *exps.front().item;
        auto result = evaluateExpressions ? Expression(left == *exps.back().item) : Expression();

        exps.clear();

        if (expect(Lexer::TokenType::Equality)) {
          state.internalValue = std::move(result);
          rulesToIgnore.push_back(RuleType::Equality);
          return RuleType::Expression;
        }

        return result;
      }
    }},
    { RuleType::String, [&](Parser::RuleState& state, std::vector<Expectation>& exps) -> RuleReturn {
      auto str = expect(Lexer::TokenType::String);
      if (!str) return ALTACORE_NULLOPT;
      if (!evaluateExpressions) return Expression();
      return Expression(AltaCore::Util::unescape(str.token.raw.substr(1, str.token.raw.length() - 2)));
    }},
    { RuleType::Retrieval, [&](Parser::RuleState& state, std::vector<Expectation>& exps) -> RuleReturn {
      auto target = expect(Lexer::TokenType::Identifier);
      if (!target) return ALTACORE_NULLOPT;
      if (!evaluateExpressions) return Expression();
      if (definitions.find(target.token.raw) == definitions.end()) {
        return Expression();
      } else {
        return definitions[target.token.raw];
      }
    }},
    { RuleType::MacroCall, [&](Parser::RuleState& state, std::vector<Expectation>& exps) -> RuleReturn {
      if (state.internalIndex == 0) {
        auto target = expect(Lexer::TokenType::Identifier);
        if (!target) return ALTACORE_NULLOPT;
        if (!expect(Lexer::TokenType::OpeningParenthesis)) return ALTACORE_NULLOPT;
        state.internalIndex = 1;
        state.internalValue = std::make_tuple(rulesToIgnore, target.token.raw, std::vector<Expression>());
        rulesToIgnore.clear();
        return RuleType::Expression;
      } else {
        auto& [ignores, target, args] = std::any_cast<std::tuple<decltype(rulesToIgnore), std::string, std::vector<Expression>>>(state.internalValue);
        rulesToIgnore = ignores;

        if (exps.back()) {
          args.push_back(*exps.back().item);
          if (expect(Lexer::TokenType::Comma)) {
            ignores = rulesToIgnore;
            return RuleType::Expression;
          }
        }

        if (!expect(Lexer::TokenType::ClosingParenthesis)) return ALTACORE_NULLOPT;

        if (!evaluateExpressions) return Expression();

        if (target == "defined") {
          return defined(args);
        } else {
          throw std::runtime_error("only builtin macros are currently supported");
        }
      }
    }},
    { RuleType::BooleanLiteral, [&](Parser::RuleState& state, std::vector<Expectation>& exps) -> RuleReturn {
      auto id = expect(Lexer::TokenType::Identifier);
      if (!id) return ALTACORE_NULLOPT;
      if (!evaluateExpressions) return Expression();
      if (id.token.raw == "true") {
        return Expression(true);
      } else if (id.token.raw == "false") {
        return Expression(false);
      } else {
        return ALTACORE_NULLOPT;
      }
    }},
    { RuleType::And, [&](Parser::RuleState& state, std::vector<Expectation>& exps) -> RuleReturn {
      if (state.internalIndex == 0) {
        rulesToIgnore.push_back(RuleType::And);
        state.internalIndex = 1;
        return RuleType::Expression;
      } else if (state.internalIndex == 1) {
        rulesToIgnore.pop_back();

        if (!exps.back()) return ALTACORE_NULLOPT;

        if (!expect(Lexer::TokenType::And)) return ALTACORE_NULLOPT;

        rulesToIgnore.push_back(RuleType::And);
        state.internalIndex = 2;
        return RuleType::Expression;
      } else {
        rulesToIgnore.pop_back();

        if (!exps.back()) return ALTACORE_NULLOPT;

        auto& left = (exps.size() == 1) ? std::any_cast<Expression>(state.internalValue) : *exps.front().item;
        auto result = evaluateExpressions ? Expression(left && *exps.back().item) : Expression();
        if (!result) {
          evaluateExpressions = false;
        }

        exps.clear();

        if (expect(Lexer::TokenType::And)) {
          state.internalValue = std::move(result);
          rulesToIgnore.push_back(RuleType::And);
          return RuleType::Expression;
        }

        return result;
      }
    }},
    { RuleType::Or, [&](Parser::RuleState& state, std::vector<Expectation>& exps) -> RuleReturn {
      if (state.internalIndex == 0) {
        rulesToIgnore.push_back(RuleType::Or);
        state.internalIndex = 1;
        return RuleType::Expression;
      } else if (state.internalIndex == 1) {
        rulesToIgnore.pop_back();

        if (!exps.back()) return ALTACORE_NULLOPT;

        if (!expect(Lexer::TokenType::Or)) return ALTACORE_NULLOPT;

        rulesToIgnore.push_back(RuleType::Or);
        state.internalIndex = 2;
        return RuleType::Expression;
      } else {
        rulesToIgnore.pop_back();

        if (!exps.back()) return ALTACORE_NULLOPT;

        auto& left = (exps.size() == 1) ? std::any_cast<Expression>(state.internalValue) : *exps.front().item;
        auto result = evaluateExpressions ? Expression(left || *exps.back().item) : Expression();
        if (result) {
          evaluateExpressions = false;
        }

        exps.clear();

        if (expect(Lexer::TokenType::Or)) {
          state.internalValue = std::move(result);
          rulesToIgnore.push_back(RuleType::Or);
          return RuleType::Expression;
        }

        return result;
      }
    }},
    { RuleType::Wrapped, [&](Parser::RuleState& state, std::vector<Expectation>& exps) -> RuleReturn {
      if (state.internalIndex == 0) {
        if (!expect(Lexer::TokenType::OpeningParenthesis)) return ALTACORE_NULLOPT;
        state.internalIndex = 1;
        state.internalValue = rulesToIgnore;
        rulesToIgnore.clear();
        return RuleType::Expression;
      } else {
        rulesToIgnore = std::any_cast<decltype(rulesToIgnore)>(state.internalValue);
        if (!exps.back()) return ALTACORE_NULLOPT;
        if (!expect(Lexer::TokenType::ClosingParenthesis)) return ALTACORE_NULLOPT;
        return exps.back().item;
      }
    }},
  };
  */
};

AltaCore::Preprocessor::ExpressionParser::RuleReturn AltaCore::Preprocessor::ExpressionParser::runRule(RuleType rule, Parser::RuleState& state, std::vector<Expectation>& exps) {
  if (rule == RuleType::Expression) {
    if (state.iteration == 0) {
      return std::initializer_list<ExpectationType> {
        RuleType::Wrapped,
          RuleType::Or,
          RuleType::And,
          RuleType::Equality,
          RuleType::MacroCall,
          RuleType::BooleanLiteral,
          RuleType::Retrieval,
          RuleType::String,
      };
    }
    if (!exps[0]) return ALTACORE_NULLOPT;
    return exps[0].item;
  } else if (rule == RuleType::Equality) {
    if (state.internalIndex == 0) {
      rulesToIgnore.push_back(RuleType::Equality);
      state.internalIndex = 1;
      return RuleType::Expression;
    } else if (state.internalIndex == 1) {
      rulesToIgnore.pop_back();

      if (!exps.back()) return ALTACORE_NULLOPT;

      if (!expect(Lexer::TokenType::Equality)) return ALTACORE_NULLOPT;

      rulesToIgnore.push_back(RuleType::Equality);
      state.internalIndex = 2;
      return RuleType::Expression;
    } else {
      rulesToIgnore.pop_back();

      if (!exps.back()) return ALTACORE_NULLOPT;

      auto& left = (exps.size() == 1) ? std::any_cast<Expression>(state.internalValue) : *exps.front().item;
      auto result = evaluateExpressions ? Expression(left == *exps.back().item) : Expression();

      exps.clear();

      if (expect(Lexer::TokenType::Equality)) {
        state.internalValue = std::move(result);
        rulesToIgnore.push_back(RuleType::Equality);
        return RuleType::Expression;
      }

      return result;
    }
  } else if (rule == RuleType::MacroCall) {
    if (state.internalIndex == 0) {
      auto target = expect(Lexer::TokenType::Identifier);
      if (!target) return ALTACORE_NULLOPT;
      if (!expect(Lexer::TokenType::OpeningParenthesis)) return ALTACORE_NULLOPT;
      state.internalIndex = 1;
      state.internalValue = std::make_tuple(rulesToIgnore, target.token.raw, std::vector<Expression>());
      rulesToIgnore.clear();
      return RuleType::Expression;
    } else {
      auto& [ignores, target, args] = std::any_cast<std::tuple<decltype(rulesToIgnore), std::string, std::vector<Expression>>>(state.internalValue);
      rulesToIgnore = ignores;

      if (exps.back()) {
        args.push_back(*exps.back().item);
        if (expect(Lexer::TokenType::Comma)) {
          ignores = rulesToIgnore;
          return RuleType::Expression;
        }
      }

      if (!expect(Lexer::TokenType::ClosingParenthesis)) return ALTACORE_NULLOPT;

      if (!evaluateExpressions) return Expression();

      if (target == "defined") {
        return defined(args);
      } else {
        throw std::runtime_error("only builtin macros are currently supported");
      }
    }
  } else if (rule == RuleType::Retrieval) {
    auto target = expect(Lexer::TokenType::Identifier);
    if (!target) return ALTACORE_NULLOPT;
    if (!evaluateExpressions) return Expression();
    if (definitions.find(target.token.raw) == definitions.end()) {
      return Expression();
    } else {
      return definitions[target.token.raw];
    }
  } else if (rule == RuleType::String) {
    auto str = expect(Lexer::TokenType::String);
    if (!str) return ALTACORE_NULLOPT;
    if (!evaluateExpressions) return Expression();
    return Expression(AltaCore::Util::unescape(str.token.raw.substr(1, str.token.raw.length() - 2)));
  } else if (rule == RuleType::BooleanLiteral) {
    auto id = expect(Lexer::TokenType::Identifier);
    if (!id) return ALTACORE_NULLOPT;
    if (!evaluateExpressions) return Expression();
    if (id.token.raw == "true") {
      return Expression(true);
    } else if (id.token.raw == "false") {
      return Expression(false);
    } else {
      return ALTACORE_NULLOPT;
    }
  } else if (rule == RuleType::And) {
    if (state.internalIndex == 0) {
      rulesToIgnore.push_back(RuleType::And);
      state.internalIndex = 1;
      return RuleType::Expression;
    } else if (state.internalIndex == 1) {
      rulesToIgnore.pop_back();

      if (!exps.back()) return ALTACORE_NULLOPT;

      if (!expect(Lexer::TokenType::And)) return ALTACORE_NULLOPT;

      rulesToIgnore.push_back(RuleType::And);
      state.internalIndex = 2;
      return RuleType::Expression;
    } else {
      rulesToIgnore.pop_back();

      if (!exps.back()) return ALTACORE_NULLOPT;

      auto& left = (exps.size() == 1) ? std::any_cast<Expression>(state.internalValue) : *exps.front().item;
      auto result = evaluateExpressions ? Expression(left && *exps.back().item) : Expression();
      if (!result) {
        evaluateExpressions = false;
      }

      exps.clear();

      if (expect(Lexer::TokenType::And)) {
        state.internalValue = std::move(result);
        rulesToIgnore.push_back(RuleType::And);
        return RuleType::Expression;
      }

      return result;
    }
  } else if (rule == RuleType::Or) {
    if (state.internalIndex == 0) {
      rulesToIgnore.push_back(RuleType::Or);
      state.internalIndex = 1;
      return RuleType::Expression;
    } else if (state.internalIndex == 1) {
      rulesToIgnore.pop_back();

      if (!exps.back()) return ALTACORE_NULLOPT;

      if (!expect(Lexer::TokenType::Or)) return ALTACORE_NULLOPT;

      rulesToIgnore.push_back(RuleType::Or);
      state.internalIndex = 2;
      return RuleType::Expression;
    } else {
      rulesToIgnore.pop_back();

      if (!exps.back()) return ALTACORE_NULLOPT;

      auto& left = (exps.size() == 1) ? std::any_cast<Expression>(state.internalValue) : *exps.front().item;
      auto result = evaluateExpressions ? Expression(left || *exps.back().item) : Expression();
      if (result) {
        evaluateExpressions = false;
      }

      exps.clear();

      if (expect(Lexer::TokenType::Or)) {
        state.internalValue = std::move(result);
        rulesToIgnore.push_back(RuleType::Or);
        return RuleType::Expression;
      }

      return result;
    }
  } else if (rule == RuleType::Wrapped) {
    if (state.internalIndex == 0) {
      if (!expect(Lexer::TokenType::OpeningParenthesis)) return ALTACORE_NULLOPT;
      state.internalIndex = 1;
      state.internalValue = rulesToIgnore;
      rulesToIgnore.clear();
      return RuleType::Expression;
    } else {
      rulesToIgnore = std::any_cast<decltype(rulesToIgnore)>(state.internalValue);
      if (!exps.back()) return ALTACORE_NULLOPT;
      if (!expect(Lexer::TokenType::ClosingParenthesis)) return ALTACORE_NULLOPT;
      return exps.back().item;
    }
  }

  return ALTACORE_NULLOPT;
};

/*
ALTACORE_OPTIONAL<AltaCore::Preprocessor::Expression> AltaCore::Preprocessor::ExpressionParser::runRule(AltaCore::Preprocessor::RuleType rule) {
  using TokenType = Lexer::TokenType;
  if (rule == RuleType::Expression) {
    // lowest to highest precedence
    // note: `Wrapped` is an exception. since it
    //       requires an opening parenthesis as it's first
    //       expectation for it to become a valid expectation,
    //       it can be detected right away
    auto exp = expect({
      RuleType::Wrapped,
      RuleType::Or,
      RuleType::And,
      RuleType::Equality,
      RuleType::MacroCall,
      RuleType::BooleanLiteral,
      RuleType::Retrieval,
      RuleType::String,
    });
    if (!exp) return ALTACORE_NULLOPT;
    return exp.item;
  } else if (rule == RuleType::Equality) {
    rulesToIgnore.push_back(RuleType::Equality);
    auto left = expect(RuleType::Expression);
    rulesToIgnore.pop_back();
    if (!left) return ALTACORE_NULLOPT;
    
    if (!expect(TokenType::Equality)) return ALTACORE_NULLOPT;

    rulesToIgnore.push_back(RuleType::Equality);
    auto right = expect(RuleType::Expression);
    rulesToIgnore.pop_back();
    if (!right) return ALTACORE_NULLOPT;

    auto result = evaluateExpressions ? Expression(*left.item == *right.item) : Expression();

    while (expect(TokenType::Equality)) {
      rulesToIgnore.push_back(RuleType::Equality);
      auto right = expect(RuleType::Expression);
      rulesToIgnore.pop_back();
      if (!right) break;
      if (evaluateExpressions) {
        result = Expression(result == *right.item);
      }
    }

    return result;
  } else if (rule == RuleType::MacroCall) {
    auto target = expect(TokenType::Identifier);
    if (!target) return ALTACORE_NULLOPT;
    if (!expect(TokenType::OpeningParenthesis)) return ALTACORE_NULLOPT;
    std::vector<Expression> arguments;
    bool closed = false;
    if (expect(TokenType::ClosingParenthesis)) {
      // optimization for no-argument calls
      closed = true;
    } else {
      Expectation arg = expect(RuleType::Expression);
      while (arg) {
        arguments.push_back(*arg.item);
        if (!expect(TokenType::Comma)) break;
        arg = expect(RuleType::Expression);
      }
      expect(TokenType::Comma); // optional trailing comma
    }
    if (!closed && !expect(TokenType::ClosingParenthesis)) return ALTACORE_NULLOPT;
    if (!evaluateExpressions) return Expression();
    if (target.token.raw == "defined") {
      return defined(arguments);
    } else {
      throw std::runtime_error("only builtin macros are currently supported");
    }
  } else if (rule == RuleType::Retrieval) {
    auto target = expect(TokenType::Identifier);
    if (!target) return ALTACORE_NULLOPT;
    if (!evaluateExpressions) return Expression();
    if (definitions.find(target.token.raw) == definitions.end()) {
      return Expression();
    } else {
      return definitions[target.token.raw];
    }
  } else if (rule == RuleType::String) {
    auto str = expect(TokenType::String);
    if (!str) return ALTACORE_NULLOPT;
    if (!evaluateExpressions) return Expression();
    return Expression(AltaCore::Util::unescape(str.token.raw.substr(1, str.token.raw.length() - 2)));
  } else if (rule == RuleType::BooleanLiteral) {
    auto id = expect(TokenType::Identifier);
    if (!id) return ALTACORE_NULLOPT;
    if (!evaluateExpressions) return Expression();
    if (id.token.raw == "true") {
      return Expression(true);
    } else if (id.token.raw == "false") {
      return Expression(false);
    }
  } else if (rule == RuleType::And) {
    rulesToIgnore.push_back(RuleType::And);
    auto left = expect(RuleType::Expression);
    rulesToIgnore.pop_back();
    if (!left) return ALTACORE_NULLOPT;

    if (!expect(TokenType::And)) return ALTACORE_NULLOPT;

    rulesToIgnore.push_back(RuleType::And);
    auto right = expect(RuleType::Expression);
    rulesToIgnore.pop_back();
    if (!right) return ALTACORE_NULLOPT;

    auto result = evaluateExpressions ? Expression(*left.item && *right.item) : Expression();
    if (!result) {
      evaluateExpressions = false;
    }

    while (expect(TokenType::And)) {
      rulesToIgnore.push_back(RuleType::And);
      auto right = expect(RuleType::Expression);
      rulesToIgnore.pop_back();
      if (!right) break;
      if (evaluateExpressions) {
        result = Expression(result && *right.item);
        if (!result) {
          evaluateExpressions = false;
        }
      }
    }

    evaluateExpressions = true;
    return result;
  } else if (rule == RuleType::Or) {
    rulesToIgnore.push_back(RuleType::Or);
    auto left = expect(RuleType::Expression);
    rulesToIgnore.pop_back();
    if (!left) return ALTACORE_NULLOPT;

    if (!expect(TokenType::Or)) return ALTACORE_NULLOPT;

    rulesToIgnore.push_back(RuleType::Or);
    auto right = expect(RuleType::Expression);
    rulesToIgnore.pop_back();
    if (!right) return ALTACORE_NULLOPT;

    auto result = evaluateExpressions ? Expression(*left.item || *right.item) : Expression();
    if (result) {
      evaluateExpressions = false;
    }

    while (expect(TokenType::Or)) {
      rulesToIgnore.push_back(RuleType::Or);
      auto right = expect(RuleType::Expression);
      rulesToIgnore.pop_back();
      if (!right) break;
      if (evaluateExpressions) {
        result = Expression(result || *right.item);
        if (result) {
          evaluateExpressions = false;
        }
      }
    }

    evaluateExpressions = true;
    return result;
  } else if (rule == RuleType::Wrapped) {
    if (!expect(TokenType::OpeningParenthesis)) return ALTACORE_NULLOPT;
    auto exp = expect(RuleType::Expression);
    if (!exp) return ALTACORE_NULLOPT;
    if (!expect(TokenType::ClosingParenthesis)) return ALTACORE_NULLOPT;
    return exp.item;
  }
  return ALTACORE_NULLOPT;
};
*/

void AltaCore::Preprocessor::ExpressionParser::parse() {
  Expectation exp = expect(RuleType::Expression);
  if (!exp.valid) return;
  root = exp.item;
};

AltaCore::Preprocessor::Expression AltaCore::Preprocessor::Preprocessor::evaluateExpression(std::string expressionString) {
  Lexer::Lexer lexer;
  lexer.feed(expressionString);

  ExpressionParser parser(lexer.tokens, definitions);
  parser.parse();

  if (auto val = *parser.root) {
    return val;
  } else {
    return Expression(nullptr);
  }
};

void AltaCore::Preprocessor::Preprocessor::feed(std::string chunk) {
  auto& result = fileResults[filePath.toString()];

  auto lines = PreprocessorUtils::split(lineCache + chunk, '\n');
  auto allTokens = PreprocessorUtils::splitLinesByWhitespace(lineCache + chunk, true, true);
  std::vector<std::string> flat;
  std::vector<size_t> lineStartIndexes;
  for (auto& tokens: allTokens) {
    lineStartIndexes.push_back(flat.size());
    flat.insert(flat.end(), tokens.begin(), tokens.end());
  }
  std::vector<uint8_t> untouchableLines;

  for (size_t i = 0; i < lines.size(); i++) {
    auto& line = lines[i];
    auto& tokens = allTokens[i];
    bool processIt = true;
    if (tokens.size() < 1) {
      // empty line
      untouchableLines.push_back(0);
      continue;
    }
    if (tokens[0].substr(0, 2) == "##") {
      auto directive = tokens[0].substr(2);
      if (
        directive == "if" ||
        directive == "else" ||
        directive == "end" ||
        directive == "define" ||
        directive == "undefine"
      ) {
        processIt = false;
      }
    }
    untouchableLines.push_back(processIt ? 0 : 2);
  }

  // `- 1` because we ignore the last line
  for (size_t k = 0; k < lines.size() - 1; k++) {
    auto line = lines[k];
    auto tokens = allTokens[k];
    auto untouchable = untouchableLines[k];
    auto lineStartIndex = lineStartIndexes[k];
    auto saveForLater = [&]() {
      // make sure we don't save the same chunk more than once
      if (lineCache.length() < chunk.length() || lineCache.substr(lineCache.length() - chunk.length(), chunk.length()) != chunk) {
        lineCache += chunk;
      }
    };
    if (untouchable == 2) {
      auto directive = tokens[0].substr(2);
      PreprocessorUtils::Index token(tokens, 0);
      if (directive == "if") {
        auto result = evaluateExpression(line.substr(4));
        auto ok = !!result;
        lastConditionalResults.push(ok);
        enteredConditionals.push(ok);
        conditionals.push("");
      } else if (directive == "else") {
        if (token.hasNext() && token.peek() == "if") {
          auto block = conditionals.top();
          auto ok = lastConditionalResults.top();
          auto enteredConditional = enteredConditionals.top();
          if (!ok && !enteredConditional) {
            std::vector<std::string> slice(tokens.begin() + 2, tokens.end());
            auto result = evaluateExpression(PreprocessorUtils::join(slice, " "));
            lastConditionalResults.top() = !!result;
            conditionals.top() = "";
            enteredConditionals.top() = !!result;
          } else {
            lastConditionalResults.top() = false;
          }
        } else {
          auto block = conditionals.top();
          auto ok = lastConditionalResults.top();
          auto enteredConditional = enteredConditionals.top();
          if (!ok && !enteredConditional) {
            lastConditionalResults.top() = true;
            conditionals.top() = "";
            enteredConditionals.top() = true;
          } else {
            lastConditionalResults.top() = false;
          }
        }
      } else if (directive == "end") {
        if (token.hasNext() && token.peek() == "if") {
          token.next();
          auto block = conditionals.top();
          auto ok = lastConditionalResults.top();
          auto enteredConditional = enteredConditionals.top();
          conditionals.pop();
          lastConditionalResults.pop();
          enteredConditionals.pop();
          if (enteredConditional) {
            auto& target = (conditionals.size() > 0) ? conditionals.top() : result;
            target += block;
          }
        }
      } else if (directive == "define") {
        if (token.hasNext()) {
          token.next();
          auto target = *token;
          if (PreprocessorUtils::isIdentifier(target)) {
            if (token.hasNext()) {
              std::vector<std::string> slice(tokens.begin() + 2, tokens.end());
              definitions[target] = evaluateExpression(PreprocessorUtils::join(slice, " "));
            } else {
              definitions[target] = Expression(nullptr);
            }
          }
        }
      } else if (directive == "undefine") {
        if (token.hasNext()) {
          token.next();
          auto target = *token;
          if (PreprocessorUtils::isIdentifier(target)) {
            if (definitions.find(target) != definitions.end()) {
              definitions.erase(target);
            }
          }
        }
      }
    } else if (!untouchable && !fallThrough()) {
      auto& target = (conditionals.size() > 0) ? conditionals.top() : result;
      std::vector<size_t> importLocations;
      std::vector<std::string> imports; // only the imports on this line and any that start on this line and spill over onto following lines
      for (size_t i = lineStartIndex; i < flat.size(); i++) {
        PreprocessorUtils::Index tok(flat, i);
        if (*tok != "import") continue;
        if (canSaveForLater && !tok.hasNext()) {
          return saveForLater();
        } else if (tok.hasNext() && tok.peek()[0] == '"' && tok.peek()[tok.peek().length() - 1] == '"') {
          tok.next();
          auto importRequest = tok;
          if (canSaveForLater && !tok.hasNext()) {
            return saveForLater();
          } else if (tok.hasNext() && tok.peek() != "as") {
            continue;
          }
          tok.next();
          if (canSaveForLater && !tok.hasNext()) {
            return saveForLater();
          } else if (tok.hasNext() && !PreprocessorUtils::isIdentifier(tok.peek())) {
            continue;
          }
          tok.next();
          importLocations.push_back(i);
          imports.push_back(importRequest->substr(1, importRequest->length() - 2));
          i = tok.i + 1;
          continue;
        }
        if (tok.hasNext() && tok.peek() == "{") {
          tok.next();
        }
        bool hasIt = true;
        while (tok.hasNext()) {
          if (tok.peek().substr(tok.peek().length() - 1) != ",") {
            if (tok.peek() == "from" || tok.peek() == "}") {
              break;
            }
            if (!PreprocessorUtils::isIdentifier(tok.peek())) {
              hasIt = false;
              break;
            }
            tok.next();
            if (canSaveForLater && !tok.hasNext()) {
              return saveForLater();
            } else if (tok.hasNext() && tok.peek() == "as") {
              tok.next();
              auto aliasStr = tok.peek();
              bool next = false;
              if (aliasStr[aliasStr.length() - 1] == ',') {
                next = true;
                aliasStr = aliasStr.substr(0, aliasStr.length() - 1);
              }
              if (!PreprocessorUtils::isIdentifier(aliasStr)) {
                hasIt = false;
                break;
              }
              tok.next();
              if (next) continue;
            }
            if (canSaveForLater && !tok.hasNext()) {
              return saveForLater();
            } else if (tok.hasNext() && tok.peek() == ",") {
              tok.next(); // skip the next token (the comma)
              continue;
            }
            break;
          } else if (!PreprocessorUtils::isIdentifier(tok.peek().substr(0, tok.peek().length() - 1))) {
            hasIt = false;
            break;
          }
          tok.next();
        }
        if (!hasIt) continue;
        if (tok.hasNext() && tok.peek() == "}") {
          tok.next();
        }
        if (canSaveForLater && !tok.hasNext()) {
          return saveForLater();
        } else if (tok.hasNext() && tok.peek() != "from") {
          continue;
        }
        tok.next();
        if (canSaveForLater && !tok.hasNext()) {
          return saveForLater();
        } else if (tok.hasNext() && tok.peek()[0] != '"' && tok.peek()[tok.peek().length() - 1] != '"') {
          continue;
        }
        tok.next();
        importLocations.push_back(i);
        imports.push_back(tok->substr(1, tok->length() - 2));
        i = tok.i + 1;
        if (i >= lineStartIndexes[k + 1]) break;
      }
      auto delimiters = PreprocessorUtils::findSubstitutionDelimiters(line);
      size_t nextStart = 0;
      for (size_t i = 0; i < imports.size(); i++) {
        auto& importStart = importLocations[i];
        auto& importRequest = imports[i];
        for (size_t j = 0; j < delimiters.size(); j++) {
          auto& [delimiterStart, delimiterEnd] = delimiters[j];
          if (delimiterEnd + lineStartIndex < importStart) {
            // [comment id: p.1]
            // TODO
            auto defName = line.substr(delimiterStart + 1, delimiterEnd - delimiterStart - 2);
            target += line.substr(nextStart, delimiterStart - nextStart + 1);
            if (definitions.find(defName) != definitions.end()) {
              auto& val = definitions[defName];
              if (val.type == ExpressionType::String) {
                target += val.string;
              } else if (val.type == ExpressionType::Boolean) {
                target += (val.boolean) ? "true" : "false";
              } else {
                target += "";
              }
            } else {
              throw std::runtime_error("definition not found: " + defName);
            }
            nextStart = delimiterEnd + 1;
            delimiters.erase(delimiters.begin() + j);
          } else {
            // the delimiters are sorted in ascending start order, so if this delimiter
            // comes after the current import, all following tokens will as well
            // so we can just stop here
            break;
          }
        }
        Preprocessor pre(Filesystem::Path(), definitions, fileResults, fileReader);
        fileReader(*this, pre, importRequest);
      }
      for (auto& [delimiterStart, delimiterEnd]: delimiters) {
        // see comment p.1 above
        auto defName = line.substr(delimiterStart + 2, delimiterEnd - delimiterStart - 2);
        target += line.substr(nextStart, delimiterStart - nextStart);
        if (definitions.find(defName) != definitions.end()) {
          auto& val = definitions[defName];
          if (val.type == ExpressionType::String) {
            target += val.string;
          } else if (val.type == ExpressionType::Boolean) {
            target += (val.boolean) ? "true" : "false";
          } else {
            target += "";
          }
        } else {
          throw std::runtime_error("definition not found: " + defName);
        }
        nextStart = delimiterEnd + 1;
      }
      delimiters.clear();
      target += line.substr(nextStart);
    }
    result += '\n';
  }

  lineCache = lines[lines.size() - 1];
};

void AltaCore::Preprocessor::Preprocessor::done() {
  canSaveForLater = false;
  feed("");
  fileResults[filePath.toString()] += lineCache;
};

void AltaCore::Preprocessor::defaultFileReader(AltaCore::Preprocessor::Preprocessor& orig, AltaCore::Preprocessor::Preprocessor& newPre, std::string importRequest) {
  auto path = Modules::resolve(importRequest, orig.filePath);
  newPre.filePath = path;
  std::ifstream file(path.toString());
  std::string line;

  if (!file.is_open()) {
    throw std::runtime_error("Couldn't open the input file!");
  }

  while (std::getline(file, line)) {
    if (file.peek() != EOF) {
      line += "\n";
    }
    newPre.feed(line);
  }

  file.close();
  newPre.done();
};
