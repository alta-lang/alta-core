#include "../include/altacore/lexer.hpp"
#include <string.h>
#include <fstream>

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
          if (!first) {
            if (
              !foundBase && (
                character == 'b' || character == 'B' ||
                character == 'o' || character == 'O' ||
                character == 'x' || character == 'X'
              )
            ) {
              foundBase = true;
              return true;
            }
            if (foundBase && ((character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z'))) {
              return true;
            }
          }
        } break;
        case TokenType::Decimal: {
          *contigious = true;

          if (character == '.' && tokens.back().type == TokenType::Dot) {
            return false;
          }

          if (first) {
            foundDecimalPoint = false;
            foundFraction = false;
          }

          if (character >= '0' && character <= '9') {
            if (foundDecimalPoint) {
              foundFraction = true;
            }
            return true;
          } else if (character == '.') {
            if (foundDecimalPoint) {
              return false;
            }
            foundDecimalPoint = true;
            return true;
          } else {
            if (foundDecimalPoint && foundFraction) {
              *ended = true;
            }
            return false;
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
        case TokenType::Code: {
          *contigious = true;
          if (backticksFound < 3) {
            if (character != '`') return false;
            ++backticksFound;
            if (backticksFound == 2 || backticksFound == 3) {
              if (currentLine != lastPosition.first || currentColumn != lastPosition.second + 1) {
                backticksFound = 1;
              }
            }
            if (backticksFound == 1 || backticksFound == 2) {
              lastPosition = { currentLine, currentColumn };
            }
            return true;
          } else {
            if (character == '`') {
              ++backticksFound;
              if (backticksFound == 5 || backticksFound == 6) {
                if (currentLine != lastPosition.first || currentColumn != lastPosition.second + 1) {
                  backticksFound = 4;
                }
              }
              if (backticksFound == 4 || backticksFound == 5) {
                lastPosition = { currentLine, currentColumn };
              } else if (backticksFound == 6) {
                backticksFound = 0;
                *ended = true;
              }
            }
            // all other characters are included
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
            (character >= 'A' && character <= 'Z') ||
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
      token.arrayPosition = tokens.size();
      token.line = currentLine;
      token.column = currentColumn;
      token.originalLine = currentLine;
      token.originalColumn = currentColumn;
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
      token.arrayPosition = tokens.size();
      token.line = currentLine;
      token.column = currentColumn;
      token.originalLine = currentLine;
      token.originalColumn = currentColumn;
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
              foundBase = false;
              ruleIteration = 0;
            }
            backlog.pop_front();
            continue;
          } else {
            hangingRule = TokenType::None;
            foundBase = false;
            ruleIteration = 0;
            if (contigious && !ended) {
              auto& back = tokens.back();
              for (auto rit = back.raw.rbegin(); rit < back.raw.rend(); rit++) {
                backlog.push_front(*rit);
              }
              totalCount = back.position;
              currentLine = back.line;
              currentColumn = back.column - 1;
              fails[totalCount].insert(back.type);
              tokens.pop_back();
              incrementTotal = false;
              continue;
            }
          }
        }

        if (stopAfterToken) {
          auto& back = tokens.back();
          if (
            (back.raw == stopAfterToken.raw) &&
            (back.line == stopAfterToken.line + extraLines) &&
            (
              (
                (stopAfterToken.line == startLine) &&
                (back.column == stopAfterToken.column + extraColumns)
              ) ||
              (
                (back.column == stopAfterToken.column)
              )
            )
          ) {
            stopped = true;
            break;
          }

          if (
            back.line > stopAfterToken.line ||
            (
              back.line == stopAfterToken.line &&
              back.column > stopAfterToken.column
            ) ||
            (
              back.line == stopAfterToken.line &&
              back.column == stopAfterToken.column &&
              back.raw.size() > stopAfterToken.raw.size()
            )
          ) {
            bool found = false;
            for (size_t i = stopAfterTokenIndex; i < originalTokens.size(); i++) {
              auto& tok = originalTokens[i];
              if (
                tok.line > back.line ||
                (
                  tok.line == back.line &&
                  tok.column > back.column + back.raw.size()
                )
              ) {
                found = true;
                stopAfterToken = tok;
                stopAfterTokenIndex = i;
                break;
              }
            }
            if (!found) {
              stopAfterToken = Token(false);
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
    void Lexer::relex(size_t tokPos, const std::string value) {
      reset(tokPos);
      auto& replace = originalTokens[tokPos];
      int64_t origLineCount = 0;
      int64_t newLineCount = 0;
      auto origLastLine = replace.raw;
      auto newLastLine = value;
      for (size_t i = 0; i < replace.raw.size(); i++) {
        auto& character = replace.raw[i];
        if (character == '\n') {
          origLineCount++;
          origLastLine = replace.raw.substr(i + 1);
        }
      }
      for (size_t i = 0; i < value.size(); i++) {
        auto& character = value[i];
        if (character == '\n') {
          newLineCount++;
          newLastLine = value.substr(i + 1);
        }
        backlog.push_back(character);
      }
      extraLines = newLineCount - origLineCount;
      extraColumns = (int64_t)newLastLine.size() - (int64_t)origLastLine.size();

      startLine = replace.line + origLineCount;

      if (originalTokens.size() > tokPos + 1) {
        stopAfterToken = originalTokens[tokPos + 1];
        stopAfterTokenIndex = tokPos + 1;
      }
      auto path = filePath.toString();
      std::ifstream file(path);
      file.seekg(replace.position + replace.raw.size());
      
      std::string line;
      while (std::getline(file, line)) {
        if (file.peek() != EOF) {
          line += '\n';
        }
        feed(line);
        if (stopped) break;
      }

      tokens.insert(tokens.end(), originalTokens.begin() + stopAfterToken.arrayPosition + 1, originalTokens.end());

      file.close();
    };
    void Lexer::reset(size_t position) {
      fails.clear();
      hangingRule = TokenType::None;
      consumeNext = false;
      characterLiteralEscaped = false;
      backlog.clear();
      absences.clear(); // for now; TODO: clear only absences after position
      startLine = 0;
      extraLines = 0;
      extraColumns = 0;
      stopAfterToken = Token(false);
      stopAfterTokenIndex = 0;
      originalTokens.clear();
      totalCount = 0;
      currentLine = 1;
      currentColumn = 0;
      stopped = false;

      if (position < tokens.size()) {
        originalTokens = tokens;
        tokens.erase(tokens.begin() + position, tokens.end());
        auto& tok = originalTokens[position];
        currentLine = tok.line;
        currentColumn = tok.column - 1;
        totalCount = tok.position - 1;
      }
    }
  };
};
