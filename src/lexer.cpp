#include "../include/altacore/lexer.hpp"

namespace AltaCore {
  namespace Lexer {
    LexerError::LexerError(size_t _line, size_t _column):
      std::runtime_error(std::string("LexerError: Failed to lex input at ") + std::to_string(_line) + ":" + std::to_string(_column)),
      line(_line),
      column(_column)
      {};

    bool Lexer::runRule(const TokenType rule, const char character, bool first, bool* ended) {
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
          if (character == '\\') {
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
        default: {
          if (character == TokenType_simpleCharacters[(int)rule]) {
            *ended = true;
            return true;
          }
        } break;
      };
      return false;
    };

    Token& Lexer::appendNewToken(const TokenType rule, const char character, bool setHanging) {
      Token token;
      token.line = currentLine;
      token.column = currentColumn;
      token.type = rule;
      token.raw = std::string(1, character);
      tokens.push_back(token);
      if (setHanging) hangingRule = rule;
      return tokens.back();
    };
    Token& Lexer::appendNewToken(const TokenType rule, std::string data, bool setHanging) {
      Token token;
      token.line = currentLine;
      token.column = currentColumn;
      token.type = rule;
      token.raw = data;
      tokens.push_back(token);
      if (setHanging) hangingRule = rule;
      return tokens.back();
    };
    void Lexer::feed(const std::string data) {
      backlog.append(data);
      lex();
    };
    void Lexer::lex() {
      for (size_t i = 0; i < backlog.length(); i++) {
        currentColumn++;
        const char character = backlog[i];

        if (consumeNext) {
          consumeNext = false;
          Token& token = tokens.back();
          token.raw.append(1, character);
          continue;
        }

        if (hangingRule != TokenType::None) {
          bool ended = false;
          bool matches = runRule(hangingRule, character, false, &ended);
          if (matches) {
            Token& token = tokens.back();
            token.raw.append(1, character);
            if (ended) hangingRule = TokenType::None;
            continue;
          } else {
            hangingRule = TokenType::None;
          }
        }

        bool cont = false;
        for (int j = 1; j < (int)TokenType::LAST + 1; j++) {
          bool ended = false;
          if ((TokenType)j == TokenType::Equality) {
            if (character == '=' && backlog.length() > i + 1 && backlog[i + 1] == '=') {
              appendNewToken(TokenType::Equality, "==");
              cont = true;
              hangingRule = TokenType::None;
              i++; // skip the next character
              break;
            }
          } else if ((TokenType)j == TokenType::And) {
            if (character == '&' && backlog.length() > i + 1 && backlog[i + 1] == '&') {
              appendNewToken(TokenType::And, "&&");
              cont = true;
              hangingRule = TokenType::None;
              i++; // skip the next character
              break;
            }
          } else if ((TokenType)j == TokenType::Or) {
            if (character == '|' && backlog.length() > i + 1 && backlog[i + 1] == '|') {
              appendNewToken(TokenType::Or, "||");
              cont = true;
              hangingRule = TokenType::None;
              i++; // skip the next character
              break;
            }
          } else if (runRule((TokenType)j, character, true, &ended)) {
            appendNewToken((TokenType)j, character);
            cont = true;
            if (ended) hangingRule = TokenType::None;
            break;
          }
        }
        if (cont) continue;

        if (character == '\n') {
          currentLine++;
          currentColumn = 0;
          continue;
        }

        if (character == ' ' || character == '\r' || character == '\t') {
          continue;
        }

        if (throwOnAbsence) {
          throw LexerError(currentLine, currentColumn);
        } else {
          absences.push_back(std::tuple<size_t, size_t>(currentLine, currentColumn));
        }
      }

      backlog.clear();
    };
  };
};
