#ifndef ALTACORE_PARSER_HPP
#define ALTACORE_PARSER_HPP

#if defined(__has_include) && __has_include(<optional>)
#include <optional>
#define ALTACORE_OPTIONAL std::optional
#define ALTACORE_NULLOPT std::nullopt
#else
#include <optional.hpp>
#define ALTACORE_OPTIONAL tl::optional
#define ALTACORE_NULLOPT tl::nullopt
#endif

#include "variant.hpp"
#include "any.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <functional>
#include <unordered_set>
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
      FunctionCall,
      String,
      FunctionDeclaration,
      Attribute,
      GeneralAttribute,
      AnyLiteral,
      ConditionalStatement,
      VerbalConditionalExpression,
      PunctualConditonalExpression,
      Block,
      NonequalityRelationalOperation,
      EqualityRelationalOperation,
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
      bool operator ==(const GenericExpectationType<RT, TT>& other);
    };
    template<typename RT, typename TT, typename T> struct GenericExpectation {
      using ExpectationType = GenericExpectationType<RT, TT>;

      bool valid = false;
      ExpectationType type;
      ALTACORE_OPTIONAL<T> item = ALTACORE_NULLOPT;
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

    template<typename RT> class GenericState {
      public:
        size_t currentPosition = 0;
        std::vector<RT> rulesToIgnore;

        bool operator ==(const GenericState<RT>& rhs) const;
    };

    class RuleState {
      public:
        size_t iteration = 0;
        size_t internalIndex = 0;
        ALTACORE_ANY internalValue;
    };

    template<typename RT, typename TT, class T> class GenericParser {
      public:
        using Expectation = GenericExpectation<RT, TT, T>;
        using ExpectationType = GenericExpectationType<RT, TT>;
        using State = GenericState<RT>;
        using RuleReturn = ALTACORE_VARIANT<ExpectationType, std::initializer_list<ExpectationType>, ALTACORE_OPTIONAL<T>>;
        //using Rule = std::function<RuleReturn(RuleState&, std::vector<Expectation>&)>;
      private:
        std::unordered_map<RT, State> loopCache;
        std::unordered_map<size_t, std::unordered_set<RT>> failed;
        //std::unordered_map<RT, Rule> defaultRuleTable;
      protected:
        std::vector<Token> tokens;
        State currentState;
        std::vector<RT>& rulesToIgnore = currentState.rulesToIgnore;

        /*
        virtual std::unordered_map<RT, Rule>& ruleTable() {
          return defaultRuleTable;
        };
        */

        Expectation expect(std::vector<ExpectationType> expectations);
        Expectation expect(std::initializer_list<ExpectationType> expectations) {
          return expect(std::vector(expectations));
        };
        Expectation expect(ExpectationType expectation) {
          return expect({ expectation });
        };
        Expectation expectAnyToken();

        virtual RuleReturn runRule(RT, RuleState&, std::vector<Expectation>&) {
          return ALTACORE_NULLOPT;
        };
      public:
        ALTACORE_OPTIONAL<T> root;

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
        std::vector<std::shared_ptr<AST::AttributeNode>> expectAttributes();
        RuleReturn expectBinaryOperation(RuleType rule, std::vector<ExpectationType> operatorTokens, std::vector<AST::OperatorType> operatorTypes, RuleState& state, std::vector<Expectation>& expectations);
        // </helper-functions>

        std::unordered_set<std::string> typesToIgnore;
      protected:
        virtual RuleReturn runRule(RuleType, RuleState&, std::vector<Expectation>&);
      public:
        void parse();

        Parser(std::vector<Token> tokens);
    };
  };
};

#include "generic-parser.hpp" // include the GenericParser implementation

#endif // ALTACORE_PARSER_HPP
