#ifndef ALTACORE_LEXER_HPP
#define ALTACORE_LEXER_HPP

#include <stdexcept>
#include <string>
#include <vector>
#include <tuple>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <utility>

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
      SingleLineComment,
      Identifier,
      Integer,
      String,
      Character,
      // </special-rules>

      // <multi-character-rules>
      Equality,
      And,
      Or,
      Returns,
      LessThanOrEqualTo,
      GreaterThanOrEqualTo,
      Inequality,
      // </multi-character-rules>

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
      QuestionMark,
      OpeningAngleBracket,
      ClosingAngleBracket,
      Ampersand,
      OpeningSquareBracket,
      ClosingSquareBracket,

      LAST, // always keep this as last, it counts the number of items in the enum
    };

    // empty strings are for complex tokens that have special rules
    static const char* const TokenType_simpleCharacters[] = {
      "",

      "",
      "",
      "",
      "",
      "",

      "==",
      "&&",
      "||",
      "->",
      "<=",
      ">=",
      "!=",
      
      "{",
      "}",
      "(",
      ")",
      ":",
      ";",
      ",",
      "=",
      ".",
      "+",
      "-",
      "*",
      "/",
      "@",
      "?",
      "<",
      ">",
      "&",
      "[",
      "]",

      "",
    };

    static const char* const TokenType_names[] = {
      "None",

      "Single line comment",
      "Identifier",
      "Integer",
      "String",
      "Character",

      "Equality",
      "And",
      "Or",
      "Returns",
      "Less than or equal to",
      "Greater than to equal to",
      "Inequality",

      "Opening brace",
      "Closing brace",
      "Opening parenthesis",
      "Closing parenthesis",
      "Colon",
      "Semicolon",
      "Comma",
      "Equal sign",
      "Dot",
      "Plus sign",
      "Minus sign",
      "Asterisk",
      "Forward slash",
      "`At` sign",
      "Question mark",
      "Opening angle bracket",
      "Closing angle bracket",
      "Ampersand",
      "Opening square bracket",
      "Closing sqaure bracket",

      "LAST", // shouldn't be necessary, but just in case ;)
    };
    
    struct Token {
      TokenType type;
      std::string raw;
      size_t position;
      size_t originalLine;
      size_t originalColumn;
      size_t line;
      size_t column;
    };

    /**
     * A streaming lexer for Alta
     */
    class Lexer {
      private:
        std::deque<char> backlog;
        std::unordered_map<size_t, std::unordered_set<TokenType>> fails;
        TokenType hangingRule = TokenType::None;
        size_t ruleIteration = 0;
        bool consumeNext = false;

        bool characterLiteralEscaped = false;

        /**
         * this is where the majority of the actual lexer logic goes,
         * since it contains the code for evaluating our rules
         */
        bool runRule(const TokenType rule, const char character, bool first, bool* ended, bool* contigious);

        Token& appendNewToken(const TokenType rule, const char character, bool setHanging = true);
        Token& appendNewToken(const TokenType rule, std::string data, bool setHanging = true);
      public:
        bool throwOnAbsence = false;
        std::vector<Token> tokens;
        std::vector<std::tuple<size_t, size_t>> absences;
        size_t totalCount = 0;
        size_t currentLine = 1;
        size_t currentColumn = 0;
        size_t currentOriginalLine = 1;
        size_t currentOriginalColumn = 0;
        std::function<std::pair<size_t, size_t>(size_t line, size_t column)> locationLookupFunction = nullptr;

        Lexer(std::function<std::pair<size_t, size_t>(size_t line, size_t column)> _locationLookupFunction = nullptr):
          locationLookupFunction(_locationLookupFunction)
          {};

        void feed(const std::string data);
        void lex();
    };
  };
};

#endif // ALTACORE_LEXER_HPP
