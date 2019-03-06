#ifndef ALTACORE_PREPROCESSOR_HPP
#define ALTACORE_PREPROCESSOR_HPP

#include "simple-map.hpp"
#include <string>
#include <functional>
#include <stack>
#include "fs.hpp"
#include "parser.hpp"
#include "timing.hpp"

// this thing is a mess. but it works. so don't touch it too much.
// it *should* be rewritten so it can be properly maintained, but
// somehow it works, so i'm scared to mess with it. here be dragons.
// you've been warned.

namespace AltaCore {
  namespace Preprocessor {
    enum class ExpressionType {
      Boolean,
      String,
      Null,
      Undefined,
    };
    struct Expression {
      ExpressionType type;
      bool defined;
      bool boolean;
      std::string string;
      std::nullptr_t null;

      Expression(bool _boolean):
        type(ExpressionType::Boolean),
        boolean(_boolean),
        defined(true)
        {};
      Expression(const char* _string):
        type(ExpressionType::String),
        string(_string),
        defined(true)
        {};
      Expression(std::string _string):
        type(ExpressionType::String),
        string(_string),
        defined(true)
        {};
      Expression(std::nullptr_t):
        type(ExpressionType::Null),
        null(nullptr),
        defined(true)
        {};
      Expression():
        type(ExpressionType::Undefined),
        null(nullptr),
        defined(false)
        {};

      bool operator ==(const Expression& right);
      explicit operator bool();
    };
    enum class RuleType {
      Expression,
      Equality,
      String,
      Retrieval,
      MacroCall,
      BooleanLiteral,
      And,
      Or,
      Wrapped,
      AnyLiteral,
    };
    class ExpressionParser: public Parser::GenericParser<RuleType, Lexer::TokenType, Expression> {
      protected:
        ALTACORE_MAP<std::string, Expression>& definitions;
        bool evaluateExpressions = true; // for short-circuit evaluation in && and ||

        // <builtin-macros>
        Expression defined(std::vector<Expression> targets);
        // </builtin-macros>
      public:
        void parse();

        ExpressionParser(std::vector<Lexer::Token> tokens, ALTACORE_MAP<std::string, Expression>& definitions);
    };

    struct Location {
      size_t originalLine = 0;
      size_t originalColumn = 0;
      size_t newLine = 0;
      size_t newColumn = 0;
      size_t charactersContained = 0;

      Location() = default;
      Location(size_t _originalLine, size_t _originalColumn, size_t _newLine, size_t _newColumn, size_t _charactersContained):
        originalLine(_originalLine),
        originalColumn(_originalColumn),
        newLine(_newLine),
        newColumn(_newColumn),
        charactersContained(_charactersContained)
        {};
    };

    class Preprocessor;
    Filesystem::Path defaultFileResolver(Preprocessor& orig, std::string importRequest);
    void defaultFileReader(Preprocessor& orig, Preprocessor& newPre, Filesystem::Path path);
    class Preprocessor {
      private:
        std::string lineCache;
        std::function<Filesystem::Path(Preprocessor&, std::string)> fileResolver;
        std::function<void(Preprocessor&, Preprocessor&, Filesystem::Path)> fileReader;
        const bool fallThrough() {
          return (lastConditionalResults.size() > 0) && (lastConditionalResults.top() == false);
        };
        Expression evaluateExpression(std::string expression);
        std::stack<std::string> conditionals;
        std::stack<bool> enteredConditionals;
        std::stack<bool> lastConditionalResults;
        size_t totalLines = 0;
        bool canSaveForLater = true;
      public:
        Filesystem::Path filePath;
        ALTACORE_MAP<std::string, Expression>& definitions;
        ALTACORE_MAP<std::string, std::string>& fileResults;
        ALTACORE_MAP<std::string, std::vector<Location>>& locationMaps;
        void feed(std::string chunk);
        void done();

        Preprocessor(
          Filesystem::Path _filePath,
          ALTACORE_MAP<std::string, Expression>& _defs,
          ALTACORE_MAP<std::string, std::string>& _results,
          ALTACORE_MAP<std::string, std::vector<Location>>& _locationMaps,
          std::function<Filesystem::Path(Preprocessor&, std::string)> _fileResolver = defaultFileResolver,
          std::function<void(Preprocessor&, Preprocessor&, Filesystem::Path)> _fileReader = defaultFileReader
        ):
          filePath(_filePath),
          definitions(_defs),
          fileResults(_results),
          locationMaps(_locationMaps),
          fileResolver(_fileResolver),
          fileReader(_fileReader)
          {};
    };
  };
};

#endif // ALTACORE_PREPROCESSOR_HPP
