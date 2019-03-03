#include "../include/altacore/preprocessor.hpp"
#include "../include/altacore/modules.hpp"
#include "../include/altacore/lexer.hpp"
#include "../include/altacore/parser.hpp"
#include "../include/altacore/util.hpp"
#include <locale>
#include <vector>
#include <utility>
#include <fstream>
#include <algorithm>

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

AltaCore::Preprocessor::ExpressionParser::ExpressionParser(std::vector<Lexer::Token> _tokens, ALTACORE_MAP<std::string, Expression>& _definitions):
  GenericParser(_tokens),
  definitions(_definitions)
  {};

void AltaCore::Preprocessor::ExpressionParser::parse() {
  /*
  Expectation exp = expect(RuleType::Expression);
  if (!exp.valid) return;
  root = exp.item;
  */
  std::stack<RuleStackElement> ruleStack;

  ruleStack.emplace(
    RuleType::Expression,
    std::stack<RuleType>(),
    RuleState(currentState),
    std::vector<Expectation>()
  );

  auto next = [&](bool ok = false, std::vector<RuleType> rules = {}, Expression result = Expression()) {
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
        newExps.push_back(Expectation()); // push back an invalid expectation
      }
    } else if (ruleExps.size() == 0) {
      newNextExps = {};
      newExps.push_back(Expectation(oldRuleType, result));
    }
  };

  while (ruleStack.size() > 0) {
    auto& [rule, nextExps, state, exps] = ruleStack.top();

    if (nextExps.size() > 0) {
      auto nextExp = nextExps.top();
      nextExps.pop();

      ruleStack.emplace(
        nextExp,
        std::stack<RuleType>(),
        RuleState(currentState),
        std::vector<Expectation>()
      );
      continue;
    }

    #define ACP_NOT_OK { next(); continue; }
    #define ACP_NODE(x) { next(true, {}, x); continue; }
    #define ACP_RULES(x) { next(true, x); continue; }
    #define ACP_RULE_LIST(...) { next(true, { __VA_ARGS__ }); continue; }
    #define ACP_EXP(x) { if (x) { next(true, {}, *x); } else { next(false); }; continue; }
    #define ACP_RULE(x) { next(true, { RuleType::x }); continue; }

    if (rule == RuleType::Expression) {
      if (state.iteration == 0) ACP_RULE(Or);
      if (!exps.back()) ACP_NOT_OK;
      root = exps.back().item;
      
      next(true);
      break;
    } else if (rule == RuleType::Equality) {
      if (state.internalIndex == 0) {
        state.internalIndex = 1;
        ACP_RULE(MacroCall);
      } else if (state.internalIndex == 1) {
        if (!exps.back()) ACP_NOT_OK;

        if (!expect(Lexer::TokenType::Equality)) ACP_EXP(exps.back().item);

        state.internalIndex = 2;
        ACP_RULE(MacroCall);
      } else {
        if (!exps.back()) ACP_NOT_OK;

        auto left = (exps.size() == 1) ? ALTACORE_ANY_CAST<Expression>(state.internalValue) : *exps.front().item;
        auto result = evaluateExpressions ? Expression(left == *exps.back().item) : Expression();

        exps.clear();

        if (expect(Lexer::TokenType::Equality)) {
          state.internalValue = std::move(result);
          ACP_RULE(MacroCall);
        }

        ACP_NODE(result);
      }
    } else if (rule == RuleType::MacroCall) {
      if (state.internalIndex == 0) {
        State cache = currentState;
        auto target = expect(Lexer::TokenType::Identifier);
        if (!(target && expect(Lexer::TokenType::OpeningParenthesis))) {
          state.internalIndex = 2;
          currentState = cache;
          ACP_RULE(AnyLiteral);
        }
        state.internalIndex = 1;
        state.internalValue = std::make_tuple(target.raw, std::vector<Expression>());
        ACP_RULE(Expression);
      } else if (state.internalIndex == 1) {
        auto [target, args] = ALTACORE_ANY_CAST<std::tuple<std::string, std::vector<Expression>>>(state.internalValue);

        if (exps.back()) {
          args.push_back(*exps.back().item);
          if (expect(Lexer::TokenType::Comma)) {
            ACP_RULE(Expression);
          }
        }

        if (!expect(Lexer::TokenType::ClosingParenthesis)) ACP_NOT_OK;

        if (!evaluateExpressions) ACP_NODE(Expression());

        if (target == "defined") {
          ACP_NODE(defined(args));
        } else {
          throw std::runtime_error("only builtin macros are currently supported");
        }
      } else {
        ACP_EXP(exps.back().item);
      }
    } else if (rule == RuleType::Retrieval) {
      auto target = expect(Lexer::TokenType::Identifier);
      if (!target) ACP_NOT_OK;
      if (!evaluateExpressions) ACP_NODE(Expression());
      if (definitions.find(target.raw) == definitions.end()) {
        ACP_NODE(Expression());
      } else {
        ACP_NODE(definitions[target.raw]);
      }
    } else if (rule == RuleType::String) {
      auto str = expect(Lexer::TokenType::String);
      if (!str) ACP_NOT_OK;
      if (!evaluateExpressions) ACP_NODE(Expression());
      ACP_NODE(Expression(AltaCore::Util::unescape(str.raw.substr(1, str.raw.length() - 2))));
    } else if (rule == RuleType::BooleanLiteral) {
      auto id = expect(Lexer::TokenType::Identifier);
      if (!id) ACP_NOT_OK;
      if (!evaluateExpressions) ACP_NODE(Expression());
      if (id.raw == "true") {
        ACP_NODE(Expression(true));
      } else if (id.raw == "false") {
        ACP_NODE(Expression(false));
      } else {
        ACP_NOT_OK;
      }
    } else if (rule == RuleType::And) {
      if (state.internalIndex == 0) {
        state.internalIndex = 1;
        ACP_RULE(Equality);
      } else if (state.internalIndex == 1) {
        if (!exps.back()) ACP_NOT_OK;

        if (!expect(Lexer::TokenType::And)) ACP_EXP(exps.back().item);

        state.internalIndex = 2;
        ACP_RULE(Equality);
      } else {
        if (!exps.back()) ACP_NOT_OK;

        auto left = (exps.size() == 1) ? ALTACORE_ANY_CAST<Expression>(state.internalValue) : *exps.front().item;
        auto result = evaluateExpressions ? Expression(left && *exps.back().item) : Expression();
        if (!result) {
          evaluateExpressions = false;
        }

        exps.clear();

        if (expect(Lexer::TokenType::And)) {
          state.internalValue = std::move(result);
          ACP_RULE(Equality);
        }

        ACP_NODE(result);
      }
    } else if (rule == RuleType::Or) {
      if (state.internalIndex == 0) {
        state.internalIndex = 1;
        ACP_RULE(And);
      } else if (state.internalIndex == 1) {
        if (!exps.back()) ACP_NOT_OK;

        if (!expect(Lexer::TokenType::Or)) ACP_EXP(exps.back().item);

        state.internalIndex = 2;
        ACP_RULE(And);
      } else {
        if (!exps.back()) ACP_NOT_OK;

        auto left = (exps.size() == 1) ? ALTACORE_ANY_CAST<Expression>(state.internalValue) : *exps.front().item;
        auto result = evaluateExpressions ? Expression(left || *exps.back().item) : Expression();
        if (result) {
          evaluateExpressions = false;
        }

        exps.clear();

        if (expect(Lexer::TokenType::Or)) {
          state.internalValue = std::move(result);
          ACP_RULE(And);
        }

        ACP_NODE(result);
      }
    } else if (rule == RuleType::Wrapped) {
      if (state.internalIndex == 0) {
        if (!expect(Lexer::TokenType::OpeningParenthesis)) ACP_NOT_OK;
        state.internalIndex = 1;
        ACP_RULE(Expression);
      } else {
        if (!exps.back()) ACP_NOT_OK;
        if (!expect(Lexer::TokenType::ClosingParenthesis)) ACP_NOT_OK;
        ACP_EXP(exps.back().item);
      }
    } else if (rule == RuleType::AnyLiteral) {
      if (state.internalIndex == 0) {
        state.internalIndex = 1;
        ACP_RULE_LIST(
          RuleType::BooleanLiteral,
          RuleType::Retrieval,
          RuleType::String
        );
      }

      ACP_EXP(exps.back().item);
    }

    #undef ACP_NOT_OK
    #undef ACP_NODE
    #undef ACP_RULES
    #undef ACP_RULE_LIST
    #undef ACP_EXP
    #undef ACP_RULE

    next();
  }
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
  auto& locations = locationMaps[filePath.toString()];

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
    totalLines++;
    auto line = lines[k];
    auto tokens = allTokens[k];
    auto untouchable = untouchableLines[k];
    auto lineStartIndex = lineStartIndexes[k];
    auto strings = PreprocessorUtils::findStrings(line);
    auto slCommentStart = line.find('#');
    std::function<bool(size_t, size_t)> doComment = [](size_t, size_t) { return false; };
    if (slCommentStart != -1) {
      bool ok = true;
      for (auto [start, end]: strings) {
        if (start < slCommentStart && end > slCommentStart) {
          ok = false;
          break;
        }
      }
      if (ok) {
        doComment = [&](size_t idx, size_t nextStart) {
          if (idx < slCommentStart) return false;
          auto& target = (conditionals.size() > 0) ? conditionals.top() : result;
          auto linesInTarget = std::count(target.begin(), target.end(), '\n');
          auto lastTargetLine = target.substr(target.find_last_of('\n') + 1) + line.substr(nextStart);
          auto comment = line.substr(slCommentStart);
          locations.emplace_back(k + 1, slCommentStart + 1, linesInTarget + 1, lastTargetLine.find('#') + 1, comment.size());
          locations.emplace_back(k + 1, line.size(), linesInTarget + 1, lastTargetLine.size(), 0);
          //line = line.substr(0, slCommentStart);
          return true;
        };
      }
    }
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
          if (delimiterEnd + lineStartIndex < importStart && !doComment(delimiterEnd, nextStart)) {
            // [comment id: p.1]
            // TODO
            auto defName = line.substr(delimiterStart + 1, delimiterEnd - delimiterStart - 2);
            target += line.substr(nextStart, delimiterStart - nextStart + 1);
            if (definitions.find(defName) != definitions.end()) {
              auto& val = definitions[defName];
              std::string result = "";
              if (val.type == ExpressionType::String) {
                result = val.string;
              } else if (val.type == ExpressionType::Boolean) {
                result = (val.boolean) ? "true" : "false";
              } else {
                result = "";
              }
              auto linesInTarget = std::count(target.begin(), target.end(), '\n');
              auto lastTargetLine = target.substr(target.find_last_of('\n') + 1);
              locations.emplace_back(k + 1, delimiterStart + 1, linesInTarget + 1, lastTargetLine.size(), result.size());
              target += result;
              linesInTarget = std::count(target.begin(), target.end(), '\n');
              lastTargetLine = target.substr(target.find_last_of('\n') + 1);
              locations.emplace_back(k + 1, delimiterEnd + 1, linesInTarget + 1, lastTargetLine.size(), 0);
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
        auto path = fileResolver(*this, importRequest);
        // avoid duplicate importation
        if (fileResults.find(path.absolutify().toString()) == fileResults.end()) {
          Preprocessor pre(path, definitions, fileResults, locationMaps, fileResolver, fileReader);
          fileReader(*this, pre, path);
        }
      }
      for (auto& [delimiterStart, delimiterEnd]: delimiters) {
        if (!doComment(delimiterEnd, nextStart)) {
          // see comment p.1 above
          auto defName = line.substr(delimiterStart + 2, delimiterEnd - delimiterStart - 2);
          target += line.substr(nextStart, delimiterStart - nextStart);
          if (definitions.find(defName) != definitions.end()) {
            auto& val = definitions[defName];
            std::string result = "";
            if (val.type == ExpressionType::String) {
              result = val.string;
            } else if (val.type == ExpressionType::Boolean) {
              result = (val.boolean) ? "true" : "false";
            } else {
              result = "";
            }
            auto linesInTarget = std::count(target.begin(), target.end(), '\n');
            auto lastTargetLine = target.substr(target.find_last_of('\n') + 1);
            locations.emplace_back(totalLines, delimiterStart + 1, linesInTarget + 1, lastTargetLine.size(), result.size());
            target += result;
            linesInTarget = std::count(target.begin(), target.end(), '\n');
            lastTargetLine = target.substr(target.find_last_of('\n') + 1);
            locations.emplace_back(totalLines, delimiterEnd + 1, linesInTarget + 1, lastTargetLine.size(), 0);
          } else {
            throw std::runtime_error("definition not found: " + defName);
          }
          nextStart = delimiterEnd + 1;
        }
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
  totalLines = 0;
};

AltaCore::Filesystem::Path AltaCore::Preprocessor::defaultFileResolver(Preprocessor& orig, std::string importRequest) {
  return AltaCore::Modules::resolve(importRequest, orig.filePath);
};

void AltaCore::Preprocessor::defaultFileReader(AltaCore::Preprocessor::Preprocessor& orig, AltaCore::Preprocessor::Preprocessor& newPre, Filesystem::Path path) {
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
