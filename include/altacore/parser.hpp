#ifndef ALTACORE_PARSER_HPP
#define ALTACORE_PARSER_HPP

#include "optional.hpp"
#include "variant.hpp"
#include "any.hpp"
#include "simple-map.hpp"
#include <unordered_set>
#include <vector>
#include <functional>
#include <unordered_set>
#include "lexer.hpp"
#include "ast.hpp"
#include "timing.hpp"
#include "modules.hpp"

namespace AltaCore {
  namespace Parser {
    using Lexer::TokenType;
    using Lexer::Token;

    enum class PrepoExpressionType {
      Boolean,
      String,
      Null,
      Undefined,
    };

    class PrepoExpression {
      public:
        PrepoExpressionType type;
        bool defined;
        bool boolean;
        std::string string;
        std::nullptr_t null;

        PrepoExpression(bool _boolean):
          type(PrepoExpressionType::Boolean),
          boolean(_boolean),
          defined(true)
          {};
        PrepoExpression(const char* _string):
          type(PrepoExpressionType::String),
          string(_string),
          defined(true)
          {};
        PrepoExpression(std::string _string):
          type(PrepoExpressionType::String),
          string(_string),
          defined(true)
          {};
        PrepoExpression(std::nullptr_t):
          type(PrepoExpressionType::Null),
          null(nullptr),
          defined(true)
          {};
        PrepoExpression():
          type(PrepoExpressionType::Undefined),
          null(nullptr),
          defined(false)
          {};

        bool operator ==(const PrepoExpression& right);
        explicit operator bool();
        operator std::string();
    };

    namespace Prepo {
      PrepoExpression defined(std::vector<PrepoExpression> targets);
    };

    enum class ModifierTargetType: uint8_t {
      Function,
      Variable,
      Type,
      ClassStatement,
      Class,
      TypeAlias,
      Structure,
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
        "generic",
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
        "generic",
      },
      {
        "export",
      },
      {
        "literal",
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
      Assignment,
      AdditionOrSubtraction,
      MultiplicationOrDivisionOrModulo,
      ModuleOnlyStatement,
      Import,
      BooleanLiteral,
      FunctionCallOrSubscriptOrAccessorOrPostIncDec,
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
      //PointerOrDereference,
      WhileLoop,
      Cast,
      Character,
      TypeAlias,
      SuperClassFetch,
      Instanceof,
      Generic,
      NullRule,
      ForLoop,
      RangedFor,
      //Not,
      Accessor,
      NotOrPointerOrDereferenceOrPreIncDecOrPlusMinusOrBitNot,
      Sizeof,
      And,
      Or,
      Shift,
      BitwiseAnd,
      BitwiseOr,
      BitwiseXor,
      DecimalLiteral,
      Structure,
      Export,
      VariableDeclaration,
      Alias,
      Delete,
      ControlDirective,
    };

    enum class PrepoRuleType {
      Root,
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

    class State {
      public:
        size_t currentPosition = 0;

        bool operator ==(const State& rhs) const;
    };

    class RuleState {
      public:
        size_t iteration = 0;
        size_t internalIndex = 0;
        State stateAtStart;
        State currentState;
        ALTACORE_ANY internalValue;

        RuleState(State _stateAtStart):
          stateAtStart(_stateAtStart),
          currentState(_stateAtStart)
          {};
    };

    class Parser {
      public:
        using NodeType = std::shared_ptr<AST::Node>;

        using Expectation = GenericExpectation<RuleType, NodeType>;
        using ExpectationType = GenericExpectationType<RuleType, TokenType>;
        using RuleReturn = ALTACORE_VARIANT<ExpectationType, std::initializer_list<ExpectationType>, ALTACORE_OPTIONAL<NodeType>>;

        using RuleStackElement = std::tuple<RuleType, std::stack<RuleType>, RuleState, std::vector<Expectation>, std::shared_ptr<AST::Node>, std::tuple<State, std::stack<bool>, std::stack<bool>, bool>>;
      private:
        using NextFunctionType = std::function<void(bool, std::vector<RuleType>, NodeType)>;
        using SaveStateType = std::function<void()>;
        using RestoreStateType = std::function<void()>;

        using PrepoExpectation = GenericExpectation<PrepoRuleType, PrepoExpression>;
        using PrepoRuleStackElement = std::tuple<PrepoRuleType, std::stack<PrepoRuleType>, RuleState, std::vector<PrepoExpectation>, PrepoExpression, State>;

        bool evaluateExpressions = true;

        // <helper-functions>
        ALTACORE_OPTIONAL<std::string> expectModifier(ModifierTargetType mtt);
        std::vector<std::string> expectModifiers(ModifierTargetType mtt);
        bool expectKeyword(std::string keyword);
        std::vector<std::shared_ptr<AST::AttributeNode>> expectAttributes();
        // this got a little out of hand 🤔
        bool expectBinaryOperation(
          RuleType rule,
          RuleType nextHigherPrecedentRule,
          std::vector<std::vector<TokenType>> operatorTokens,
          std::vector<AST::OperatorType> operatorTypes,
          RuleState& state,
          std::vector<Expectation>& expectations,
          std::shared_ptr<AST::Node>& ruleNode,
          NextFunctionType next,
          SaveStateType saveState,
          RestoreStateType restoreState
        );
        ALTACORE_OPTIONAL<PrepoExpression> expectPrepoExpression();
        std::vector<Token> expectSequence(std::vector<TokenType> expectations, bool exact = true);
        // </helper-functions>

        std::unordered_set<std::string> typesToIgnore;
        Filesystem::Path filePath;

        bool inClass = false;

        std::vector<Token> tokens;
        std::vector<Token> originalTokens;
        State currentState;

        Token expect(std::vector<TokenType> expectations, bool rawPrepo = false);
        Token expect(TokenType expectation) {
          return expect(std::vector<TokenType> { expectation });
        };
        Token expectAnyToken();
        Token peek(size_t lookahead = 0, bool lookbehind = false);

        Lexer::Lexer relexer = Lexer::Lexer(Filesystem::Path());
      public:
        ALTACORE_OPTIONAL<NodeType> root;
        RuleState farthestRule = RuleState(currentState);
        ALTACORE_MAP<std::string, PrepoExpression>& definitions;

        void parse();
        void reset() {
          currentState = State();
          root = nullptr;
        };

        Parser(std::vector<Token> tokens, ALTACORE_MAP<std::string, PrepoExpression>& definitions, Filesystem::Path filePath = Filesystem::Path());
    };
  };
};

#include "generic-parser.hpp" // include the GenericParser implementation

#endif // ALTACORE_PARSER_HPP
