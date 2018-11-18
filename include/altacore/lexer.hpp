#ifndef ALTACORE_LEXER_HPP
#define ALTACORE_LEXER_HPP

#include <stdexcept>
#include <string>
#include <vector>
#include <tuple>

namespace AltaCore {
  namespace Lexer {
    class LexerError: public std::runtime_error {
      public:
        size_t line;
        size_t column;

        LexerError(size_t line, size_t column);
    };

    // btw, order matters here
    enum class TokenType {
      None,
      
      // <special-rules>
      Identifier,
      Integer,
      String,
      Equality,
      And,
      Or,
      // </special-rules>

      OpeningBrace,
      ClosingBrace,
      OpeningParenthesis,
      ClosingParenthesis,
      Colon,
      Semicolon,
      Comma,
      EqualSign,
      Dot,
      PlusSign,
      MinusSign,
      Asterisk,
      ForwardSlash,
      AtSign, // or Ampersat?
      LAST, // always keep this as last, it counts the number of items in the enum
    };

    // '\0' is for complex characters that have special rules
    static const char TokenType_simpleCharacters[] = {
      '\0',
      '\0',
      '\0',
      '\0',
      '\0',
      '\0',
      '\0',
      '{',
      '}',
      '(',
      ')',
      ':',
      ';',
      ',',
      '=',
      '.',
      '+',
      '-',
      '*',
      '/',
      '@',
      '\0',
    };

    static const char* const TokenType_names[] = {
      "None",
      "Identifier",
      "Integer",
      "String",
      "Equality",
      "And",
      "Or",
      "Opening brace",
      "Closing brace",
      "Opening parenthesis",
      "Closing parenthesis",
      "Colon",
      "Semicolon",
      "Comma",
      "EqualSign",
      "Dot",
      "AtSign",
      "LAST", // shouldn't be necessary, but just in case ;)
    };
    
    struct Token {
      TokenType type;
      std::string raw;
      size_t line;
      size_t column;
    };

    /**
     * A streaming lexer for Alta
     */
    class Lexer {
      private:
        std::string backlog;
        TokenType hangingRule = TokenType::None;
        bool consumeNext = false;

        /**
         * this is where the majority of the actual lexer logic goes,
         * since it contains the code for evaluating our rules
         */
        bool runRule(const TokenType rule, const char character, bool first, bool* ended);
        Token& appendNewToken(const TokenType rule, const char character, bool setHanging = true);
        Token& appendNewToken(const TokenType rule, std::string data, bool setHanging = true);
      public:
        bool throwOnAbsence = false;
        std::vector<Token> tokens;
        std::vector<std::tuple<size_t, size_t>> absences;
        size_t currentLine = 1;
        size_t currentColumn = 0;

        void feed(const std::string data);
        void lex();
    };
  };
};

#endif // ALTACORE_LEXER_HPP
