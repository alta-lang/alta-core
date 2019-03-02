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
      ClassStatement,
      Class,
      TypeAlias,
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
        "signed",
        "unsigned",
        "long",
        "short",
      },
      {
        "public",
        "private",
      },
      {
        "literal",
        "export",
      },
      {
        "export",
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
      FunctionCallOrSubscript,
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
      GroupedExpression,
      ClassDefinition,
      ClassStatement,
      ClassMember,
      ClassMethod,
      ClassSpecialMethod,
      ClassInstantiation,
      StrictAccessor,
      PointerOrDereference,
      WhileLoop,
      Cast,
      Character,
      TypeAlias,
      SuperClassFetch,
      Instanceof,
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
    template<typename RT, typename T> struct GenericExpectation {
      using ExpectationType = RT;

      bool valid = false;
      ExpectationType type;
      ALTACORE_OPTIONAL<T> item = ALTACORE_NULLOPT;

      GenericExpectation():
        valid(false)
        {};
      GenericExpectation(ExpectationType _type, T _item):
        valid(true),
        type(_type),
        item(_item)
        {};

      explicit operator bool() const {
        return valid;
      };
    };

    template<typename RT> class GenericState {
      public:
        size_t currentPosition = 0;

        bool operator ==(const GenericState<RT>& rhs) const;
    };

    template<typename S> class GenericRuleState {
      public:
        size_t iteration = 0;
        size_t internalIndex = 0;
        S stateAtStart;
        S currentState;
        ALTACORE_ANY internalValue;

        GenericRuleState(S _stateAtStart):
          stateAtStart(_stateAtStart),
          currentState(_stateAtStart)
          {};
    };

    template<typename RT, typename TT, class T> class GenericParser {
      public:
        using RuleType = RT;
        using TokenType = TT;
        using NodeType = T;

        using Expectation = GenericExpectation<RuleType, NodeType>;
        using ExpectationType = GenericExpectationType<RuleType, TokenType>;
        using State = GenericState<RuleType>;
        using RuleState = GenericRuleState<State>;
        using RuleReturn = ALTACORE_VARIANT<ExpectationType, std::initializer_list<ExpectationType>, ALTACORE_OPTIONAL<NodeType>>;

        using RuleStackElement = std::tuple<RuleType, std::stack<RuleType>, RuleState, std::vector<Expectation>>;

      protected:
        std::vector<Token> tokens;
        State currentState;

        Token expect(std::vector<TokenType> expectations);
        Token expect(std::initializer_list<TokenType> expectations) {
          return expect(std::vector(expectations));
        };
        Token expect(TokenType expectation) {
          return expect({ expectation });
        };
        Token expectAnyToken();

      public:
        ALTACORE_OPTIONAL<NodeType> root;
        RuleState farthestRule = RuleState(currentState);

        virtual void parse() {};
        void reset() {
          currentState = State();
          root = nullptr;
        };

        GenericParser(std::vector<Token> _tokens):
          tokens(_tokens)
          {};
    };

    class Parser: public GenericParser<RuleType, TokenType, std::shared_ptr<AST::Node>> {
      private:
        using NextFunctionType = std::function<void(bool, std::vector<RuleType>, NodeType)>;

        // <helper-functions>
        ALTACORE_OPTIONAL<std::string> expectModifier(ModifierTargetType mtt);
        std::vector<std::string> expectModifiers(ModifierTargetType mtt);
        bool expectKeyword(std::string keyword);
        std::vector<std::shared_ptr<AST::AttributeNode>> expectAttributes();
        bool expectBinaryOperation(
          RuleType rule,
          RuleType nextHigherPrecedentRule,
          std::vector<TokenType> operatorTokens,
          std::vector<AST::OperatorType> operatorTypes,
          RuleState& state,
          std::vector<Expectation>& expectations,
          NextFunctionType next
        );
        // </helper-functions>

        std::unordered_set<std::string> typesToIgnore;
        Filesystem::Path filePath;

        bool inClass = false;

      public:
        void parse();

        Parser(std::vector<Token> tokens, Filesystem::Path filePath = Filesystem::Path());
    };
  };
};

#include "generic-parser.hpp" // include the GenericParser implementation

#endif // ALTACORE_PARSER_HPP
