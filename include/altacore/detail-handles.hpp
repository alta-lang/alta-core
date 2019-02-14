#ifndef ALTACORE_DETAIL_HANDLES_HPP
#define ALTACORE_DETAIL_HANDLES_HPP

#if defined(__has_include) && __has_include(<optional>)
#include <optional>
#define ALTACORE_OPTIONAL std::optional
#define ALTACORE_NULLOPT std::nullopt
#else
#include <optional.hpp>
#define ALTACORE_OPTIONAL tl::optional
#define ALTACORE_NULLOPT tl::nullopt
#endif

#include "det.hpp"
#include "variant.hpp"
#include "attributes.hpp"
#include <vector>

namespace AltaCore {
  namespace AST {
    class ClassSpecialMethodDefinitionStatement;
    class ExpressionNode;
    class RootNode;
  };
  namespace DetailHandles {
    #define ALTACORE_DH_CTOR(x, y) public: x(decltype(inputScope) _inputScope = nullptr): y(_inputScope) {}
    #define ALTACORE_DH_SIMPLE_ALIAS(x, y) class x: public y { ALTACORE_DH_CTOR(x, y); };

    class Node {
      public:
      virtual ~Node() = default;

      std::shared_ptr<DET::Scope> inputScope = nullptr;

      Node(decltype(inputScope) _inputScope = nullptr):
        inputScope(_inputScope)
        {};
    };

    ALTACORE_DH_SIMPLE_ALIAS(ExpressionNode, Node);
    ALTACORE_DH_SIMPLE_ALIAS(StatementNode, Node);
    ALTACORE_DH_SIMPLE_ALIAS(ClassStatementNode, Node);

    ALTACORE_DH_SIMPLE_ALIAS(LiteralNode, ExpressionNode);

    ALTACORE_DH_SIMPLE_ALIAS(BooleanLiteralNode, LiteralNode);
    ALTACORE_DH_SIMPLE_ALIAS(IntegerLiteralNode, LiteralNode);
    ALTACORE_DH_SIMPLE_ALIAS(StringLiteralNode, LiteralNode);
    ALTACORE_DH_SIMPLE_ALIAS(CharacterLiteralNode, LiteralNode);
    
    // forward declaration
    class Accessor;
    class AssignmentExpression;
    class AttributeNode;
    class AttributeStatement;
    class BinaryOperation;
    class BlockNode;
    class CastExpression;
    class ClassDefinitionNode;
    class ClassInstitutionExpression;
    class ClassMemberDefinitionStatement;
    class ClassMethodDefinitionStatement;
    class ClassReadAccessorDefinitionStatement;
    class ClassSpecialMethodDefinitionStatement;
    class ConditionalExpression;
    class ConditionalStatement;
    class DereferenceExpression;
    class ExpressionStatement;
    class Fetch;
    class FunctionCallExpression;
    class FunctionDeclarationNode;
    class FunctionDefinitionNode;
    class ImportStatement;
    class Parameter;
    class PointerExpression;
    class ReturnDirectiveNode;
    class RootNode;
    class TypeAliasStatement;
    class Type;
    class VariableDefinitionExpression;
    class WhileLoopStatement;

    class Accessor: public ExpressionNode {
      ALTACORE_DH_CTOR(Accessor, ExpressionNode);

      std::shared_ptr<ExpressionNode> target = nullptr;

      bool accessesNamespace = false;
      std::vector<std::shared_ptr<DET::ScopeItem>> items;
      std::shared_ptr<DET::Function> readAccessor = nullptr;
      std::shared_ptr<DET::Function> writeAccessor = nullptr;
      std::shared_ptr<DET::ScopeItem> narrowedTo = nullptr;
      std::shared_ptr<DET::Type> targetType = nullptr;
    };

    class AssignmentExpression: public ExpressionNode {
      ALTACORE_DH_CTOR(AssignmentExpression, ExpressionNode);

      std::shared_ptr<ExpressionNode> target = nullptr;
      std::shared_ptr<ExpressionNode> value = nullptr;
    };
    class AttributeNode: public Node {
      ALTACORE_DH_CTOR(AttributeNode, Node);

      std::vector<std::shared_ptr<LiteralNode>> arguments;

      std::shared_ptr<AST::Node> target = nullptr;
      std::shared_ptr<Node> targetInfo = nullptr;
      std::weak_ptr<DET::Module> module;
      std::vector<Attributes::AttributeArgument> attributeArguments;
      ALTACORE_OPTIONAL<Attributes::Attribute> attribute = ALTACORE_NULLOPT;
    };
    class AttributeStatement: public StatementNode {
      ALTACORE_DH_CTOR(AttributeStatement, StatementNode);

      std::shared_ptr<AttributeNode> attribute = nullptr;
    };
    class BinaryOperation: public ExpressionNode {
      ALTACORE_DH_CTOR(BinaryOperation, ExpressionNode);

      std::shared_ptr<ExpressionNode> left = nullptr;
      std::shared_ptr<ExpressionNode> right = nullptr;
    };
    class BlockNode: public StatementNode {
      ALTACORE_DH_CTOR(BlockNode, StatementNode);
      
      std::vector<std::shared_ptr<StatementNode>> statements;
    };
    class CastExpression: public ExpressionNode {
      ALTACORE_DH_CTOR(CastExpression, ExpressionNode);

      std::shared_ptr<ExpressionNode> target = nullptr;
      std::shared_ptr<Type> type = nullptr;
    };
    class ClassDefinitionNode: public StatementNode {
      ALTACORE_DH_CTOR(ClassDefinitionNode, StatementNode);

      std::vector<std::shared_ptr<ClassStatementNode>> statements;

      bool isExport = false;
      bool isLiteral = false;
      bool createDefaultConstructor = false;
      std::shared_ptr<AST::ClassSpecialMethodDefinitionStatement> defaultConstructor = nullptr;
      std::shared_ptr<ClassSpecialMethodDefinitionStatement> defaultConstructorDetail = nullptr;
      std::shared_ptr<DET::Class> klass = nullptr;
    };
    class ClassInstantiationExpression: public ExpressionNode {
      ALTACORE_DH_CTOR(ClassInstantiationExpression, ExpressionNode);

      std::shared_ptr<ExpressionNode> target = nullptr;
      std::vector<std::shared_ptr<ExpressionNode>> arguments;

      std::shared_ptr<DET::Function> constructor = nullptr;
      std::shared_ptr<DET::Class> klass = nullptr;
      std::unordered_map<size_t, size_t> argumentMap;
      std::vector<ALTACORE_VARIANT<std::pair<std::shared_ptr<AST::ExpressionNode>, std::shared_ptr<ExpressionNode>>, std::vector<std::pair<std::shared_ptr<AST::ExpressionNode>, std::shared_ptr<ExpressionNode>>>>> adjustedArguments;
    };
    class ClassMemberDefinitionStatement: public ClassStatementNode {
      ALTACORE_DH_CTOR(ClassMemberDefinitionStatement, ClassStatementNode);

      std::shared_ptr<VariableDefinitionExpression> varDef = nullptr;
    };
    class ClassMethodDefinitionStatement: public ClassStatementNode {
      ALTACORE_DH_CTOR(ClassMethodDefinitionStatement, ClassStatementNode);

      std::shared_ptr<FunctionDefinitionNode> funcDef = nullptr;
    };
    class ClassReadAccessorDefinitionStatement: public ClassStatementNode {
      ALTACORE_DH_CTOR(ClassReadAccessorDefinitionStatement, ClassStatementNode);

      std::shared_ptr<Type> type = nullptr;
      std::shared_ptr<BlockNode> body = nullptr;

      std::shared_ptr<DET::Scope> bodyScope = nullptr;
      std::shared_ptr<DET::Function> function = nullptr;
    };
    class ClassSpecialMethodDefinitionStatement: public ClassStatementNode {
      ALTACORE_DH_CTOR(ClassSpecialMethodDefinitionStatement, ClassStatementNode);

      std::vector<std::shared_ptr<Parameter>> parameters;
      std::shared_ptr<BlockNode> body = nullptr;

      std::shared_ptr<DET::Class> klass = nullptr;
      std::shared_ptr<DET::Function> method = nullptr;
    };
    class ConditionalExpression: public ExpressionNode {
      ALTACORE_DH_CTOR(ConditionalExpression, ExpressionNode);

      std::shared_ptr<ExpressionNode> test = nullptr;
      std::shared_ptr<ExpressionNode> primaryResult = nullptr;
      std::shared_ptr<ExpressionNode> secondaryResult = nullptr;
    };
    class ConditionalStatement: public StatementNode {
      ALTACORE_DH_CTOR(ConditionalStatement, StatementNode);

      std::shared_ptr<ExpressionNode> primaryTest = nullptr;
      std::shared_ptr<StatementNode> primaryResult = nullptr;
      std::vector<std::pair<std::shared_ptr<ExpressionNode>, std::shared_ptr<StatementNode>>> alternatives;
      std::shared_ptr<StatementNode> finalResult = nullptr;

      std::shared_ptr<DET::Scope> primaryScope = nullptr;
      std::vector<std::shared_ptr<DET::Scope>> alternativeScopes;
      std::shared_ptr<DET::Scope> finalScope = nullptr;
    };
    class DereferenceExpression: public ExpressionNode {
      ALTACORE_DH_CTOR(DereferenceExpression, ExpressionNode);

      std::shared_ptr<ExpressionNode> target = nullptr;
    };
    class ExpressionStatement: public StatementNode {
      ALTACORE_DH_CTOR(ExpressionStatement, StatementNode);

      std::shared_ptr<ExpressionNode> expression = nullptr;
    };
    class Fetch: public ExpressionNode {
      ALTACORE_DH_CTOR(Fetch, ExpressionNode);

      std::vector<std::shared_ptr<DET::ScopeItem>> items;
      std::shared_ptr<DET::ScopeItem> narrowedTo;
    };
    class FunctionCallExpression: public ExpressionNode {
      ALTACORE_DH_CTOR(FunctionCallExpression, ExpressionNode);

      std::shared_ptr<ExpressionNode> target = nullptr;
      std::vector<std::shared_ptr<ExpressionNode>> arguments;

      bool isMethodCall = false;
      std::shared_ptr<AST::ExpressionNode> methodClassTarget = nullptr;
      std::shared_ptr<ExpressionNode> methodClassTargetInfo = nullptr;
      std::shared_ptr<DET::Type> targetType = nullptr;
      std::unordered_map<size_t, size_t> argumentMap;
      std::vector<ALTACORE_VARIANT<std::pair<std::shared_ptr<AST::ExpressionNode>, std::shared_ptr<ExpressionNode>>, std::vector<std::pair<std::shared_ptr<AST::ExpressionNode>, std::shared_ptr<ExpressionNode>>>>> adjustedArguments;
    };
    class FunctionDeclarationNode: public StatementNode {
      ALTACORE_DH_CTOR(FunctionDeclarationNode, StatementNode);

      std::vector<std::shared_ptr<Parameter>> parameters;
      std::shared_ptr<Type> returnType = nullptr;

      std::shared_ptr<DET::Function> function = nullptr;
    };
    class FunctionDefinitionNode: public StatementNode {
      ALTACORE_DH_CTOR(FunctionDefinitionNode, StatementNode);

      std::vector<std::shared_ptr<Parameter>> parameters;
      std::shared_ptr<Type> returnType = nullptr;
      std::shared_ptr<BlockNode> body = nullptr;
      std::vector<std::shared_ptr<AttributeNode>> attributes;

      std::shared_ptr<DET::Function> function = nullptr;
    };
    class ImportStatement: public StatementNode {
      ALTACORE_DH_CTOR(ImportStatement, StatementNode);

      std::shared_ptr<DET::Module> parentModule = nullptr;
      std::shared_ptr<DET::Module> importedModule = nullptr;
      std::shared_ptr<AST::RootNode> importedAST = nullptr;
      std::vector<std::shared_ptr<DET::ScopeItem>> importedItems;
    };
    class Parameter: public Node {
      ALTACORE_DH_CTOR(Parameter, Node);

      std::shared_ptr<Type> type = nullptr;
      std::vector<std::shared_ptr<AttributeNode>> attributes;
    };
    class PointerExpression: public ExpressionNode {
      ALTACORE_DH_CTOR(PointerExpression, ExpressionNode);

      std::shared_ptr<ExpressionNode> target = nullptr;
    };
    class ReturnDirectiveNode: public StatementNode {
      ALTACORE_DH_CTOR(ReturnDirectiveNode, StatementNode);

      std::shared_ptr<ExpressionNode> expression = nullptr;

      std::shared_ptr<DET::Type> functionReturnType = nullptr;
    };
    class RootNode: public Node {
      ALTACORE_DH_CTOR(RootNode, Node);

      std::vector<std::shared_ptr<StatementNode>> statements;

      std::shared_ptr<DET::Module> module = nullptr;
      std::vector<std::shared_ptr<AST::RootNode>> dependencyASTs;
      std::shared_ptr<RootNode> parent = nullptr;
    };
    class TypeAliasStatement: public StatementNode {
      ALTACORE_DH_CTOR(TypeAliasStatement, StatementNode);

      std::shared_ptr<Type> type = nullptr;

      bool isExport = false;
    };
    class Type: public Node {
      ALTACORE_DH_CTOR(Type, Node);

      std::shared_ptr<Type> returnType = nullptr;
      std::vector<std::shared_ptr<Type>> parameters;
      std::shared_ptr<ExpressionNode> lookup = nullptr;

      std::shared_ptr<DET::Type> type = nullptr;
      bool isAny = false;
      bool isFunction = false;
      bool isNative = true;
    };
    class VariableDefinitionExpression: public ExpressionNode {
      ALTACORE_DH_CTOR(VariableDefinitionExpression, ExpressionNode);
      
      std::shared_ptr<Type> type = nullptr;
      std::shared_ptr<ExpressionNode> initializationExpression = nullptr;

      std::shared_ptr<DET::Variable> variable = nullptr;
    };
    class WhileLoopStatement: public StatementNode {
      ALTACORE_DH_CTOR(WhileLoopStatement, StatementNode);

      std::shared_ptr<ExpressionNode> test = nullptr;
      std::shared_ptr<StatementNode> body = nullptr;

      std::shared_ptr<DET::Scope> scope = nullptr;
    };

    #undef ALTACORE_DH_CTOR
  };
  namespace DH = DetailHandles;
};

#endif /* ALTACORE_DETAIL_HANDLES_HPP */
