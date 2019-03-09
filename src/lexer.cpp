#include "../include/altacore/lexer.hpp"
#include <string.h>

namespace AltaCore {
  namespace Lexer {
    bool Lexer::runRule(const TokenType rule, const char character, bool first, bool* ended, bool* contigious) {
      switch (rule) {
        case TokenType::Identifier: {
          if (
            character == '_' || 
            (character >= 'A' && character <= 'Z') ||
            (character >= 'a' && character <= 'z') ||
            (!first && character >= '0' && character <= '9')
          ) {
            return true;
          }
        } break;
        case TokenType::Integer: {
          if (character >= '0' && character <= '9') {
            return true;
          }
        } break;
        case TokenType::String: {
          if (!first && character == '\\') {
            consumeNext = true;
            return true;
          }
          if (character == '"') {
            if (!first) {
              *ended = true;
            }
            return true;
          }
          if (!first) {
            return true;
          }
        } break;
        case TokenType::SingleLineComment: {
          if (first && character == '#') {
            return true;
          }
          if (character == '\n') {
            *ended = true;
          } else if (!first) {
            return true;
          }
        } break;
        case TokenType::Character: {
          *contigious = true;
          if (!first && character == '\\') {
            consumeNext = true;
            characterLiteralEscaped = true;
            return true;
          }
          if (character == '\'') {
            characterLiteralEscaped = false;
            if (!first) {
              *ended = true;
            }
            return true;
          }
          if (ruleIteration == 1 && !characterLiteralEscaped) {
            return true;
          }
        } break;
        case TokenType::PreprocessorDirective: {
          if (first && tokens.size() > 0 && tokens.back().line == currentLine) {
            return false;
          }

          if (first || ruleIteration == 1) {
            *contigious = true;
            return character == '#';
          }

          if ((character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z')) {
            return true;
          }
        } break;
        case TokenType::PreprocessorSubstitution: {
          *contigious = true;
          if (first) {
            return character == '@';
          }

          if (ruleIteration == 1) {
            return character == '[';
          }

          if (character == ']') {
            *ended = true;
            return true;
          }

          if (
            (character >= 'a' && character <= 'z') ||
            (character <= 'A' && character >= 'Z') ||
            (ruleIteration > 2 && character >= '0' && character <= '9')
          ) {
            return true;
          }
        } break;
        default: {
          *contigious = true;
          auto string = TokenType_simpleCharacters[(int)rule];
          auto length = strlen(string);
          
          if (length == 0) return false;
          
          auto stringChar = string[ruleIteration];

          if (character == stringChar) {
            if (ruleIteration + 1 >= length) {
              *ended = true;
            }
            return true;
          }
        } break;
      };
      return false;
    };

    Token& Lexer::appendNewToken(const TokenType rule, const char character, bool setHanging) {
      Token token;
      token.position = totalCount;
      token.line = currentLine;
      token.column = currentColumn;
      //token.originalLine = currentOriginalLine;
      //token.originalColumn = currentOriginalColumn;
      token.type = rule;
      token.raw = std::string(1, character);
      token.valid = true;
      tokens.push_back(token);
      if (setHanging) hangingRule = rule;
      return tokens.back();
    };
    Token& Lexer::appendNewToken(const TokenType rule, std::string data, bool setHanging) {
      Token token;
      token.position = totalCount;
      token.line = currentLine;
      token.column = currentColumn;
      //token.originalLine = currentOriginalLine;
      //token.originalColumn = currentOriginalColumn;
      token.type = rule;
      token.raw = data;
      token.valid = true;
      tokens.push_back(token);
      if (setHanging) hangingRule = rule;
      return tokens.back();
    };
    void Lexer::feed(const std::string data) {
      for (auto& character: data) {
        backlog.push_back(character);
      }
      lex();
      for (size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i].type == TokenType::SingleLineComment) {
          tokens.erase(tokens.begin() + i);
          // since our container size decreased by one,
          // we have to make sure we look at this index
          // again in the next iteration
          i--;
        }
      }
    };
    void Lexer::lex() {
      auto absoluteFilePath = filePath.absolutify();
      Timing::lexTimes[absoluteFilePath].start();

      bool incrementTotal = false;
      bool finalIncrement = backlog.size() > 0;
      while (!backlog.empty()) {
        if (!incrementTotal) {
          incrementTotal = true;
        } else {
          totalCount++;
        }

        currentColumn++;
        //currentOriginalColumn++;
        //currentLine = currentOriginalLine;
        //currentColumn = currentOriginalColumn;

        const char character = backlog.front();

        if (consumeNext) {
          consumeNext = false;
          Token& token = tokens.back();
          token.raw.append(1, character);
          backlog.pop_front();
          continue;
        }

        if (hangingRule != TokenType::None) {
          ruleIteration++;
          bool ended = false;
          bool contigious = false;
          bool matches = runRule(hangingRule, character, false, &ended, &contigious);
          if (matches) {
            Token& token = tokens.back();
            token.raw.append(1, character);
            if (ended) {
              hangingRule = TokenType::None;
              ruleIteration = 0;
            }
            backlog.pop_front();
            continue;
          } else {
            hangingRule = TokenType::None;
            ruleIteration = 0;
            if (contigious) {
              auto& back = tokens.back();
              for (auto rit = back.raw.rbegin(); rit < back.raw.rend(); rit++) {
                backlog.push_front(*rit);
              }
              totalCount = back.position;
              currentLine = back.line;
              currentColumn = back.column - 1;
              //currentOriginalLine = back.originalLine;
              //currentOriginalColumn = back.originalColumn - 1;
              fails[totalCount].insert(back.type);
              tokens.pop_back();
              incrementTotal = false;
              continue;
            }
          }
        }

        bool cont = false;
        for (int j = 1; j < (int)TokenType::LAST + 1; j++) {
          auto tokType = (TokenType)j;
          if (fails[totalCount].find(tokType) != fails[totalCount].end()) continue;
          bool ended = false;
          bool contigious = false;
          if (runRule((TokenType)j, character, true, &ended, &contigious)) {
            appendNewToken((TokenType)j, character);
            cont = true;
            if (ended) hangingRule = TokenType::None;
            break;
          } else {
            fails[totalCount].insert(tokType);
          }
        }
        if (cont) {
          backlog.pop_front();
          continue;
        }

        if (character == '\n') {
          currentLine++;
          currentColumn = 0;
          //currentOriginalLine++;
          //currentOriginalColumn = 0;
          backlog.pop_front();
          continue;
        }

        if (character == ' ' || character == '\r' || character == '\t') {
          backlog.pop_front();
          continue;
        }

        if (throwOnAbsence) {
          throw Errors::LexingError("", Errors::Position(currentLine, currentColumn, filePath));
        } else {
          absences.emplace_back(currentLine, currentColumn);
        }
      }

      if (finalIncrement) {
        totalCount++;
      }

      Timing::lexTimes[absoluteFilePath].stop();
    };
  };
};
