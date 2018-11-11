#ifndef ALTACORE_PARSER_HPP
#define ALTACORE_PARSER_HPP

#include <map>
#include <vector>
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
      },
      {
        "literal",
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
    };

    struct ExpectationType {
      bool isToken; // if true, it's a token, otherwise, it's a parser rule
      union {
        RuleType rule;
        TokenType token;
      };
      ExpectationType(RuleType _rule):
        isToken(false),
        rule(_rule)
        {};
      ExpectationType(TokenType _token):
        isToken(true),
        token(_token)
        {};
    };
    struct Expectation {
      bool valid = false;
      ExpectationType type;
      std::shared_ptr<AST::Node> item = nullptr;
      Token token;

      Expectation():
        valid(false),
        type(RuleType::None),
        item(nullptr),
        token(Token())
        {};
      Expectation(ExpectationType _type, std::shared_ptr<AST::Node> _item):
        valid(true),
        type(_type),
        item(_item),
        token(Token())
        {};
      Expectation(ExpectationType _type, AST::Node* _item):
        valid(true),
        type(_type),
        item(_item),
        token(Token())
        {};
      Expectation(ExpectationType _type, Token _token):
        valid(true),
        type(_type),
        item(nullptr),
        token(_token)
        {};
    };

    class State {
      public:
        size_t currentPosition = 0;
    };

    class Parser {
      private:
        std::vector<Token> tokens;
        State currentState;
        std::map<RuleType, size_t> loopCache;
        std::vector<RuleType> rulesToIgnore;
        /**
         * keeps an index of rules that failed at certain indexes.
         * because why try the same rule again if we already know
         * it failed at that exact spot? this improves speed incredibly
         */
        std::map<size_t, std::vector<RuleType>> failed;

        Expectation expect(std::initializer_list<ExpectationType> expectations);
        Expectation expect(ExpectationType expectation);

        // <helper-functions>
        std::vector<std::shared_ptr<AST::Parameter>> expectParameters();
        std::vector<std::string> expectModifiers(ModifierTargetType mtt);
        // </helper-functions>

        std::shared_ptr<AST::Node> runRule(RuleType rule);
      public:
        std::shared_ptr<AST::RootNode> root;

        void parse();

        Parser(std::vector<Token> _tokens):
          tokens(_tokens)
          {};
    };
  };
};

#endif // ALTACORE_PARSER_HPP
