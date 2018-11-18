#ifndef ALTACORE_PARSER_HPP
#define ALTACORE_PARSER_HPP

#include <map>
#include <vector>
#include <optional>
#include "lexer.hpp"
#include "ast.hpp"

namespace AltaCore {
  namespace Parser {
    using Lexer::TokenType;
    using Lexer::Token;

    enum class ModifierTargetType: uint8_t {
      Function,
      Variable,
      Type,
    };
    /**
     * Each entry here corresponds to a `ModifierTargetType`
     * Thus, if `ModifierTargetType::Function == 0`,
     * `modifiersForTargets[0]` == the modifiers for functions
     */
    static std::vector<const char*> modifiersForTargets[] = {
      {
        "literal",
        "export",
      },
      {
        "literal",
        "export",
      },
      {
        "ptr",
        "const",
        "ref",
      },
    };

    enum class RuleType {
      None,
      Root,
      Statement,
      Expression,
      FunctionDefinition,
      Parameter,
      Type,
      IntegralLiteral,
      ReturnDirective,
      VariableDefinition,
      Fetch,
      Accessor,
      Assignment,
      AdditionOrSubtraction,
      MultiplicationOrDivision,
      ModuleOnlyStatement,
      Import,
      BooleanLiteral,
      FrontendDirective,
      FrontendSubstitution,
    };

    template<typename RT, typename TT> struct GenericExpectationType {
      bool valid = false;
      bool isToken; // if true, it's a token, otherwise, it's a parser rule
      union {
        RT rule;
        TT token;
      };
      GenericExpectationType():
        valid(false),
        isToken(false)
        {};
      GenericExpectationType(RT _rule):
        valid(true),
        isToken(false),
        rule(_rule)
        {};
      GenericExpectationType(TT _token):
        valid(true),
        isToken(true),
        token(_token)
        {};
    };
    template<typename RT, typename TT, typename T> struct GenericExpectation {
      using ExpectationType = GenericExpectationType<RT, TT>;

      bool valid = false;
      ExpectationType type;
      std::optional<T> item = std::nullopt;
      Token token;

      GenericExpectation():
        valid(false)
        {};
      GenericExpectation(ExpectationType _type, T _item):
        valid(true),
        type(_type),
        item(_item)
        {};
      GenericExpectation(ExpectationType _type, Token _token):
        valid(true),
        type(_type),
        token(_token)
        {};
      
      explicit operator bool() const {
        return valid;
      };
    };

    class State {
      public:
        size_t currentPosition = 0;
    };

    template<typename RT, typename TT, class T> class GenericParser {
      public:
        using Expectation = GenericExpectation<RT, TT, T>;
        using ExpectationType = GenericExpectationType<RT, TT>;
      private:
        std::map<RT, size_t> loopCache;
        std::map<size_t, std::vector<RT>> failed;
      protected:
        std::vector<Token> tokens;
        std::vector<RT> rulesToIgnore;
        State currentState;

        Expectation expect(std::initializer_list<ExpectationType> expectations);
        Expectation expect(ExpectationType expectation) {
          return expect({ expectation });
        };
        Expectation expectAnyToken();

        virtual std::optional<T> runRule(RT rule) {
          return std::nullopt;
        };
      public:
        std::optional<T> root;

        virtual void parse() {};
        void reset() {
          currentState = State();
          rulesToIgnore.clear();
          failed.clear();
          loopCache.clear();
          root = nullptr;
        };

        GenericParser(std::vector<Token> _tokens):
          tokens(_tokens)
          {};
    };

    class Parser: public GenericParser<RuleType, TokenType, std::shared_ptr<AST::Node>> {
      private:
        // <helper-functions>
        std::vector<std::shared_ptr<AST::Parameter>> expectParameters();
        std::vector<std::string> expectModifiers(ModifierTargetType mtt);
        bool expectKeyword(std::string keyword);
        // </helper-functions>
      protected:
        std::optional<std::shared_ptr<AST::Node>> runRule(RuleType rule);
      public:
        void parse();

        Parser(std::vector<Token> _tokens):
          GenericParser(_tokens)
          {};
    };
  };
};

#include "generic-parser.tpp" // include the GenericParser implementation

#endif // ALTACORE_PARSER_HPP
