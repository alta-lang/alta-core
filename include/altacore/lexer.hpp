#ifndef ALTACORE_LEXER_HPP
#define ALTACORE_LEXER_HPP

#include "simple-map.hpp"
#include <stdexcept>
#include <string>
#include <vector>
#include <tuple>
#include <deque>
#include <unordered_set>
#include <functional>
#include <utility>
#include "timing.hpp"
#include "errors.hpp"

namespace AltaCore {
  namespace Lexer {
    // btw, order matters here
    enum class TokenType {
      None,
      
      // <special-rules>
      PreprocessorDirective,
      SingleLineComment,
      Identifier,
      Decimal,
      Integer,
      String,
      Character,
      PreprocessorSubstitution,
      // </special-rules>

      // <multi-character-rules>
      Equality,
      And,
      Or,
      Returns,
      LessThanOrEqualTo,
      GreaterThanOrEqualTo,
      Inequality,
      Increment,
      Decrement,
      PlusEquals,
      MinusEquals,
      TimesEquals,
      DividedEquals,
      ModuloEquals,
      LeftShift,
      LeftShiftEquals,
      RightShiftEquals,
      BitwiseAndEquals,
      BitwiseOrEquals,
      BitwiseXorEquals,
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
      ExclamationMark,
      Percent,
      Tilde,
      Caret,
      Pipe,

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
      "++",
      "--",
      "+=",
      "-=",
      "*=",
      "/=",
      "%=",
      "<<",
      "<<=",
      ">>=",
      "&=",
      "|=",
      "^=",
      
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
      "!",
      "%",
      "~",
      "^",
      "|",

      "",
    };

    static const char* const TokenType_names[] = {
      "None",

      "Preprocessor directive",
      "Single line comment",
      "Identifier",
      "Decimal",
      "Integer",
      "String",
      "Character",
      "Preprocessor substitution",

      "Equality",
      "And",
      "Or",
      "Returns",
      "Less than or equal to",
      "Greater than to equal to",
      "Inequality",
      "Increment",
      "Decrement",
      "Plus equals",
      "Minus equals",
      "Times equals",
      "Divided equals",
      "Modulo equals",
      "Left shift",
      "Right shift",
      "Left shift equals",
      "Right shift equals",
      "Bitwise AND equals",
      "Bitwise OR equals",
      "Bitwise XOR equals",

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
      "Closing square bracket",
      "Exclamation mark",
      "Percent",
      "Tilde",
      "Caret",
      "Pipe",

      "LAST", // shouldn't be necessary, but just in case ;)
    };
    
    struct Token {
      TokenType type;
      std::string raw;
      size_t position;
      size_t arrayPosition;
      size_t originalLine = SIZE_MAX;
      size_t originalColumn = SIZE_MAX;
      size_t line;
      size_t column;
      bool valid = false;

      explicit operator bool() const {
        return valid;
      }
      
      Token() {};
      Token(bool _valid):
        valid(_valid)
        {};
    };

    /**
     * A streaming lexer for Alta
     */
    class Lexer {
      private:
        std::deque<char> backlog;
        ALTACORE_MAP<size_t, std::unordered_set<TokenType>> fails;
        TokenType hangingRule = TokenType::None;
        size_t ruleIteration = 0;
        bool consumeNext = false;

        bool characterLiteralEscaped = false;
        bool foundDecimalPoint = false;
        bool foundFraction = false;

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
        std::vector<std::pair<size_t, size_t>> absences;
        size_t totalCount = 0;
        size_t currentLine = 1;
        size_t currentColumn = 0;
        Filesystem::Path filePath;

        size_t startLine = 0;
        int64_t extraLines = 0;
        int64_t extraColumns = 0;
        Token stopAfterToken = Token(false);
        size_t stopAfterTokenIndex = 0;
        std::vector<Token> originalTokens;
        bool stopped = false;

        Lexer(Filesystem::Path _filePath):
          filePath(_filePath)
          {};

        void feed(const std::string data);
        void lex();
        void relex(size_t tokPos, const std::string value);
        void reset(size_t position);
    };
  };
};

#endif // ALTACORE_LEXER_HPP
