#ifndef ALTACORE_PREPROCESSOR_HPP
#define ALTACORE_PREPROCESSOR_HPP

#include <string>
#include <functional>
#include <map>
#include <stack>
#include "fs.hpp"
#include "parser.hpp"

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
        std::map<std::string, Expression>& definitions;
        bool evaluateExpressions = true; // for short-circuit evaluation in && and ||

        // <builtin-macros>
        Expression defined(std::vector<Expression> targets);
        // </builtin-macros>

        virtual RuleReturn runRule(RuleType, RuleState&, std::vector<Expectation>&);
      public:
        void parse();

        ExpressionParser(std::vector<Lexer::Token> tokens, std::map<std::string, Expression>& definitions);
    };

    class Preprocessor;
    void defaultFileReader(Preprocessor& orig, Preprocessor& newPre, std::string importRequest);
    class Preprocessor {
      private:
        std::string lineCache;
        std::function<void(Preprocessor&, Preprocessor&, std::string)> fileReader;
        const bool fallThrough() {
          return (lastConditionalResults.size() > 0) && (lastConditionalResults.top() == false);
        };
        Expression evaluateExpression(std::string expression);
        std::stack<std::string> conditionals;
        std::stack<bool> enteredConditionals;
        std::stack<bool> lastConditionalResults;
        bool canSaveForLater = true;
      public:
        Filesystem::Path filePath;
        std::map<std::string, Expression>& definitions;
        std::map<std::string, std::string>& fileResults;
        void feed(std::string chunk);
        void done();

        Preprocessor(
          Filesystem::Path _filePath,
          std::map<std::string, Expression>& _defs,
          std::map<std::string, std::string>& _results,
          std::function<void(Preprocessor&, Preprocessor&, std::string)> _fileReader = defaultFileReader
        ):
          filePath(_filePath),
          definitions(_defs),
          fileResults(_results),
          fileReader(_fileReader)
          {};
    };
  };
};

#endif // ALTACORE_PREPROCESSOR_HPP
