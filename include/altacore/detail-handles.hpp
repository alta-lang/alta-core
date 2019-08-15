#ifndef ALTACORE_DETAIL_HANDLES_HPP
#define ALTACORE_DETAIL_HANDLES_HPP

#include "simple-map.hpp"
#include "optional.hpp"
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
    ALTACORE_DH_SIMPLE_ALIAS(NullptrExpression, ExpressionNode);

    ALTACORE_DH_SIMPLE_ALIAS(BooleanLiteralNode, LiteralNode);
    ALTACORE_DH_SIMPLE_ALIAS(IntegerLiteralNode, LiteralNode);
    ALTACORE_DH_SIMPLE_ALIAS(StringLiteralNode, LiteralNode);
    ALTACORE_DH_SIMPLE_ALIAS(CharacterLiteralNode, LiteralNode);
    ALTACORE_DH_SIMPLE_ALIAS(FloatingPointLiteralNode, LiteralNode);
    
    // forward declaration
    class Accessor;
    class AssignmentExpression;
    class AttributeNode;
    class AttributeStatement;
    class BinaryOperation;
    class UnaryOperation;
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
    class ForLoopStatement;
    class RetrievalNode;
    class SuperClassFetch;
    class GenericClassInstantiationDefinitionNode;
    class Generic;

    class RetrievalNode: public ExpressionNode {
      ALTACORE_DH_CTOR(RetrievalNode, ExpressionNode);

      std::vector<std::shared_ptr<DET::ScopeItem>> items;
    };

    class Accessor: public RetrievalNode {
      ALTACORE_DH_CTOR(Accessor, RetrievalNode);

      std::shared_ptr<ExpressionNode> target = nullptr;

      bool accessesNamespace = false;
      ALTACORE_MAP<size_t, std::vector<std::shared_ptr<DET::Class>>> parentClassAccessors;
      std::shared_ptr<DET::Function> readAccessor = nullptr;
      size_t readAccessorIndex = 0;
      std::shared_ptr<DET::Function> writeAccessor = nullptr;
      size_t writeAccessorIndex = 0;
      std::shared_ptr<DET::ScopeItem> narrowedTo = nullptr;
      size_t narrowedToIndex = 0;
      std::shared_ptr<DET::Type> targetType = nullptr;
      bool getsVariableLength = false;
      std::vector<std::shared_ptr<Type>> genericArgumentDetails;
      std::vector<std::shared_ptr<DET::Type>> genericArguments;
    };

    class AssignmentExpression: public ExpressionNode {
      ALTACORE_DH_CTOR(AssignmentExpression, ExpressionNode);

      std::shared_ptr<ExpressionNode> target = nullptr;
      std::shared_ptr<ExpressionNode> value = nullptr;
      Shared::AssignmentType type = Shared::AssignmentType::Simple;

      bool strict = false;

      std::vector<std::shared_ptr<AttributeNode>> attributes;
    };
    class AttributeNode: public Node {
      ALTACORE_DH_CTOR(AttributeNode, Node);

      std::vector<std::shared_ptr<ExpressionNode>> arguments;

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

      Shared::OperatorType type = Shared::OperatorType::Addition;
      std::shared_ptr<ExpressionNode> left = nullptr;
      std::shared_ptr<ExpressionNode> right = nullptr;
    };
    class UnaryOperation: public ExpressionNode {
      ALTACORE_DH_CTOR(UnaryOperation, ExpressionNode);

      Shared::UOperatorType type = Shared::UOperatorType::Not;
      std::shared_ptr<ExpressionNode> target = nullptr;
    };
    class BlockNode: public StatementNode {
      ALTACORE_DH_CTOR(BlockNode, StatementNode);
      
      std::vector<std::shared_ptr<StatementNode>> statements;
    };
    class CastExpression: public ExpressionNode {
      ALTACORE_DH_CTOR(CastExpression, ExpressionNode);

      std::shared_ptr<ExpressionNode> target = nullptr;
      std::shared_ptr<Type> type = nullptr;

      std::shared_ptr<DET::Type> targetType = nullptr;
      std::shared_ptr<DET::Function> fromCaster = nullptr;
      std::shared_ptr<DET::Function> toCaster = nullptr;
    };
    class ClassDefinitionNode: public StatementNode {
      ALTACORE_DH_CTOR(ClassDefinitionNode, StatementNode);

      std::vector<std::shared_ptr<ClassStatementNode>> statements;
      std::vector<std::shared_ptr<RetrievalNode>> parents;
      std::vector<std::shared_ptr<GenericClassInstantiationDefinitionNode>> genericInstantiations;

      bool isExport = false;
      bool isLiteral = false;
      bool createDefaultConstructor = false;
      bool createDefaultDestructor = false;
      bool createDefaultCopyConstructor = false;
      std::shared_ptr<AST::ClassSpecialMethodDefinitionStatement> defaultConstructor = nullptr;
      std::shared_ptr<ClassSpecialMethodDefinitionStatement> defaultConstructorDetail = nullptr;
      std::shared_ptr<DET::Class> klass = nullptr;
      std::shared_ptr<AST::ClassSpecialMethodDefinitionStatement> defaultDestructor = nullptr;
      std::shared_ptr<ClassSpecialMethodDefinitionStatement> defaultDestructorDetail = nullptr;
      std::shared_ptr<AST::ClassSpecialMethodDefinitionStatement> defaultCopyConstructor = nullptr;
      std::shared_ptr<ClassSpecialMethodDefinitionStatement> defaultCopyConstructorDetail = nullptr;
      std::vector<std::shared_ptr<Generic>> genericDetails;
    };
    class GenericClassInstantiationDefinitionNode: public ClassDefinitionNode {
      ALTACORE_DH_CTOR(GenericClassInstantiationDefinitionNode, ClassDefinitionNode);

      std::weak_ptr<ClassDefinitionNode> generic;
    };
    class ClassInstantiationExpression: public ExpressionNode {
      ALTACORE_DH_CTOR(ClassInstantiationExpression, ExpressionNode);

      std::shared_ptr<ExpressionNode> target = nullptr;
      std::vector<std::shared_ptr<ExpressionNode>> arguments;

      bool superclass = false;
      std::shared_ptr<DET::Function> constructor = nullptr;
      std::shared_ptr<DET::Class> klass = nullptr;
      ALTACORE_MAP<size_t, size_t> argumentMap;
      std::vector<ALTACORE_VARIANT<std::pair<std::shared_ptr<AST::ExpressionNode>, std::shared_ptr<ExpressionNode>>, std::vector<std::pair<std::shared_ptr<AST::ExpressionNode>, std::shared_ptr<ExpressionNode>>>>> adjustedArguments;
      bool persistent = false;
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
      std::vector<std::shared_ptr<AttributeNode>> attributes;
      std::shared_ptr<Type> specialType;

      std::shared_ptr<DET::Class> klass = nullptr;
      std::shared_ptr<DET::Function> method = nullptr;
      std::shared_ptr<DET::Function> correspondingMethod = nullptr;
      bool isCopyConstructor = false;
      bool isDefaultCopyConstructor = false;
      bool isCastConstructor = false;
    };
    class ClassOperatorDefinitionStatement: public ClassStatementNode {
      ALTACORE_DH_CTOR(ClassOperatorDefinitionStatement, ClassStatementNode);

      std::shared_ptr<BlockNode> block = nullptr;
      std::shared_ptr<Type> returnType = nullptr;
      std::shared_ptr<Type> argumentType = nullptr;
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
    class Fetch: public RetrievalNode {
      ALTACORE_DH_CTOR(Fetch, RetrievalNode);

      std::shared_ptr<DET::ScopeItem> narrowedTo;
      std::vector<std::shared_ptr<Type>> genericArgumentDetails;
      std::vector<std::shared_ptr<DET::Type>> genericArguments;
      std::shared_ptr<DET::Function> readAccessor = nullptr;
      size_t readAccessorIndex = 0;
      std::shared_ptr<DET::Function> writeAccessor = nullptr;
      size_t writeAccessorIndex = 0;

      bool referencesOutsideLambda = false;
    };
    class FunctionCallExpression: public ExpressionNode {
      ALTACORE_DH_CTOR(FunctionCallExpression, ExpressionNode);

      std::shared_ptr<ExpressionNode> target = nullptr;
      std::vector<std::shared_ptr<ExpressionNode>> arguments;

      bool isMethodCall = false;
      std::shared_ptr<AST::ExpressionNode> methodClassTarget = nullptr;
      std::shared_ptr<ExpressionNode> methodClassTargetInfo = nullptr;
      std::shared_ptr<DET::Type> targetType = nullptr;
      ALTACORE_MAP<size_t, size_t> argumentMap;
      std::vector<ALTACORE_VARIANT<std::pair<std::shared_ptr<AST::ExpressionNode>, std::shared_ptr<ExpressionNode>>, std::vector<std::pair<std::shared_ptr<AST::ExpressionNode>, std::shared_ptr<ExpressionNode>>>>> adjustedArguments;

      bool maybe = false;
    };
    class FunctionDeclarationNode: public StatementNode {
      ALTACORE_DH_CTOR(FunctionDeclarationNode, StatementNode);

      std::vector<std::shared_ptr<Parameter>> parameters;
      std::shared_ptr<Type> returnType = nullptr;

      std::shared_ptr<DET::Function> function = nullptr;
    };
    class GenericFunctionInstantiationDefinitionNode;
    class FunctionDefinitionNode: public StatementNode {
      ALTACORE_DH_CTOR(FunctionDefinitionNode, StatementNode);

      std::vector<std::shared_ptr<Parameter>> parameters;
      std::shared_ptr<Type> returnType = nullptr;
      std::shared_ptr<BlockNode> body = nullptr;
      std::vector<std::shared_ptr<AttributeNode>> attributes;

      std::vector<std::shared_ptr<GenericFunctionInstantiationDefinitionNode>> genericInstantiations;
      std::vector<std::shared_ptr<Generic>> genericDetails;

      std::shared_ptr<DET::Function> function = nullptr;
    };
    class GenericFunctionInstantiationDefinitionNode: public FunctionDefinitionNode {
      ALTACORE_DH_CTOR(GenericFunctionInstantiationDefinitionNode, FunctionDefinitionNode);

      std::weak_ptr<FunctionDefinitionNode> generic;
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

      std::shared_ptr<DET::Function> parentFunction = nullptr;
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
      std::vector<std::shared_ptr<AttributeNode>> attributes;

      bool isExport = false;
    };
    class Type: public Node {
      ALTACORE_DH_CTOR(Type, Node);

      std::shared_ptr<Type> returnType = nullptr;
      std::vector<std::shared_ptr<Type>> parameters;
      std::shared_ptr<ExpressionNode> lookup = nullptr;
      std::vector<std::shared_ptr<Type>> unionOf;

      std::shared_ptr<DET::Type> type = nullptr;
      bool isAny = false;
      bool isFunction = false;
      bool isNative = true;

      bool isOptional = false;
      std::shared_ptr<Type> optionalTarget = nullptr;
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

    class ForLoopStatement: public StatementNode {
      ALTACORE_DH_CTOR(ForLoopStatement, StatementNode);

      std::shared_ptr<ExpressionNode> initializer = nullptr;
      std::shared_ptr<ExpressionNode> condition = nullptr;
      std::shared_ptr<ExpressionNode> increment = nullptr;
      std::shared_ptr<StatementNode> body = nullptr;

      std::shared_ptr<DET::Scope> scope = nullptr;
    };

    class RangedForLoopStatement: public StatementNode {
      ALTACORE_DH_CTOR(RangedForLoopStatement, StatementNode);

      std::shared_ptr<Type> counterType = nullptr;
      std::shared_ptr<ExpressionNode> start = nullptr;
      std::shared_ptr<ExpressionNode> end = nullptr;
      std::shared_ptr<StatementNode> body = nullptr;

      std::shared_ptr<DET::Scope> scope = nullptr;
      std::shared_ptr<DET::Variable> counter = nullptr;
    };

    class SubscriptExpression: public ExpressionNode {
      ALTACORE_DH_CTOR(SubscriptExpression, ExpressionNode);

      std::shared_ptr<ExpressionNode> target = nullptr;
      std::shared_ptr<ExpressionNode> index = nullptr;
    };
    class SuperClassFetch: public ExpressionNode {
      ALTACORE_DH_CTOR(SuperClassFetch, ExpressionNode);

      //std::vector<std::shared_ptr<ExpressionNode>> arguments;

      size_t parentClassIndex = 0;
      std::shared_ptr<DET::Class> klass = nullptr;
      std::shared_ptr<DET::Class> superclass = nullptr;
      /*
      std::shared_ptr<DET::Function> constructor = nullptr;
      ALTACORE_MAP<size_t, size_t> argumentMap;
      std::vector<ALTACORE_VARIANT<std::pair<std::shared_ptr<AST::ExpressionNode>, std::shared_ptr<ExpressionNode>>, std::vector<std::pair<std::shared_ptr<AST::ExpressionNode>, std::shared_ptr<ExpressionNode>>>>> adjustedArguments;
      */
    };
    class InstanceofExpression: public ExpressionNode {
      ALTACORE_DH_CTOR(InstanceofExpression, ExpressionNode);

      std::shared_ptr<ExpressionNode> target = nullptr;
      std::shared_ptr<Type> type = nullptr;
    };
    class Generic: public Node {
      ALTACORE_DH_CTOR(Generic, Node);

      std::shared_ptr<DET::Alias> alias = nullptr;
    };
    class SizeofOperation: public ExpressionNode {
      ALTACORE_DH_CTOR(SizeofOperation, ExpressionNode);

      std::shared_ptr<Type> target = nullptr;
    };
    class StructureDefinitionStatement: public StatementNode {
      ALTACORE_DH_CTOR(StructureDefinitionStatement, StatementNode);

      bool isExternal = false;
      bool isTyped = false;
      bool isLiteral = false;
      bool isExport = false;
      std::vector<std::shared_ptr<Type>> memberTypes;
      std::vector<std::shared_ptr<AttributeNode>> attributes;

      std::shared_ptr<DET::Class> structure = nullptr;
    };
    class ExportStatement: public StatementNode {
      ALTACORE_DH_CTOR(ExportStatement, StatementNode);

      std::shared_ptr<ImportStatement> externalTarget;
      std::shared_ptr<RetrievalNode> localTarget;
    };
    class VariableDeclarationStatement: public StatementNode {
      ALTACORE_DH_CTOR(VariableDeclarationStatement, StatementNode);

      std::shared_ptr<Type> type;
      std::vector<std::shared_ptr<AttributeNode>> attributes;

      std::shared_ptr<DET::Variable> variable = nullptr;
    };
    class AliasStatement: public StatementNode {
      ALTACORE_DH_CTOR(AliasStatement, StatementNode);

      std::shared_ptr<RetrievalNode> target = nullptr;
      std::vector<std::shared_ptr<DET::Alias>> aliases;
    };
    class DeleteStatement: public StatementNode {
      ALTACORE_DH_CTOR(DeleteStatement, StatementNode);

      std::shared_ptr<ExpressionNode> target = nullptr;
      bool persistent = false;
    };
    class TryCatchBlock: public StatementNode {
      ALTACORE_DH_CTOR(TryCatchBlock, StatementNode);

      std::shared_ptr<StatementNode> tryBlock = nullptr;
      std::vector<std::pair<std::shared_ptr<Type>, std::shared_ptr<StatementNode>>> catchBlocks;
      std::shared_ptr<StatementNode> catchAllBlock = nullptr;

      std::shared_ptr<DET::Scope> tryScope = nullptr;
      std::vector<std::shared_ptr<DET::Scope>> catchScopes;
      std::shared_ptr<DET::Scope> catchAllScope = nullptr;
      std::vector<std::shared_ptr<DET::Variable>> errorVariables;
    };
    class ThrowStatement: public StatementNode {
      ALTACORE_DH_CTOR(ThrowStatement, StatementNode);

      std::shared_ptr<ExpressionNode> expression = nullptr;
    };
    class CodeLiteralNode: public StatementNode {
      ALTACORE_DH_CTOR(CodeLiteralNode, StatementNode);

      std::vector<std::shared_ptr<AttributeNode>> attributes;
    };
    class BitfieldDefinitionNode: public StatementNode {
      ALTACORE_DH_CTOR(BitfieldDefinitionNode, StatementNode);

      bool isExport = false;
      std::shared_ptr<Type> underlyingType;
      std::vector<std::shared_ptr<Type>> memberTypes;
      std::vector<std::shared_ptr<AttributeNode>> attributes;
      std::vector<std::shared_ptr<DET::Variable>> memberVariables;

      std::shared_ptr<DET::Class> bitfield = nullptr;
    };
    class LambdaExpression: public ExpressionNode {
      ALTACORE_DH_CTOR(LambdaExpression, ExpressionNode);

      std::vector<std::shared_ptr<Parameter>> parameters;
      std::shared_ptr<Type> returnType = nullptr;

      std::vector<std::shared_ptr<AttributeNode>> attributes;

      std::shared_ptr<BlockNode> body = nullptr;

      std::shared_ptr<DET::Function> function = nullptr;

      std::vector<std::shared_ptr<DET::Variable>> toCopy;
      std::vector<std::shared_ptr<DET::Variable>> toReference;
    };
    class SpecialFetchExpression: public RetrievalNode {
      ALTACORE_DH_CTOR(SpecialFetchExpression, RetrievalNode);
    };

    #undef ALTACORE_DH_CTOR
  };
  namespace DH = DetailHandles;
};

#endif /* ALTACORE_DETAIL_HANDLES_HPP */
