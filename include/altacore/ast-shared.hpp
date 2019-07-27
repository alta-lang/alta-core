#ifndef ALTACORE_AST_SHARED_HPP
#define ALTACORE_AST_SHARED_HPP

#include <cinttypes>
#include <memory>
#include "shared.hpp"

namespace AltaCore {
  namespace AST {
    using Shared::TypeModifierFlag;
    using Shared::Visibility;
    using Shared::OperatorType;
    using Shared::UOperatorType;

    enum class NodeType {
      Node,
      StatementNode,
      ExpressionNode,
      RootNode,
      ExpressionStatement,
      Type,
      Parameter,
      BlockNode,
      FunctionDefinitionNode,
      ReturnDirectiveNode,
      IntegerLiteralNode,
      VariableDefinitionExpression,
      Accessor,
      Fetch,
      AssignmentExpression,
      BooleanLiteralNode,
      BinaryOperation,
      ImportStatement,
      FunctionCallExpression,
      StringLiteralNode,
      FunctionDeclarationNode,
      AttributeNode,
      LiteralNode,
      AttributeStatement,
      ConditionalStatement,
      ConditionalExpression,
      ClassDefinitionNode,
      ClassStatementNode,
      ClassMemberDefinitionStatement,
      ClassMethodDefinitionStatement,
      ClassSpecialMethodDefinitionStatement,
      ClassInstantiationExpression,
      PointerExpression,
      DereferenceExpression,
      WhileLoopStatement,
      CastExpression,
      ClassReadAccessorDefinitionStatement,
      CharacterLiteralNode,
      TypeAliasStatement,
      SubscriptExpression,
      RetrievalNode,
      SuperClassFetch,
      InstanceofExpression,
      Generic,
      ForLoopStatement,
      RangedForLoopStatement,
      UnaryOperation,
      SizeofOperation,
      FloatingPointLiteralNode,
      StructureDefinitionStatement,
      ExportStatement,
      VariableDeclarationStatement,
      AliasStatement,
      DeleteStatement,
      ControlDirective,
      TryCatchBlock,
      ThrowStatement,
      NullptrExpression,
      CodeLiteralNode,
      BitfieldDefinitionNode,
      LambdaExpression,
    };

    static const char* const NodeType_names[] = {
      "Node",
      "StatementNode",
      "ExpressionNode",
      "RootNode",
      "ExpressionStatement",
      "Type",
      "Parameter",
      "BlockNode",
      "FunctionDefinitionNode",
      "LiteralExpressionNode",
      "ReturnDirectiveNode",
      "IntgerLiteralNode",
      "VariableDefinitionExpression",
      "Accessor",
      "Fetch",
      "AssignmentExpression",
      "BooleanLiteralNode",
      "BinaryOperation",
      "ImportStatement",
      "FunctionCallExpression",
      "StringLiteralNode",
      "FunctionDeclarationNode",
      "AttributeNode",
      "LiteralNode",
      "AttributeStatement",
      "ConditionalStatement",
      "ConditionalExpression",
      "ClassDefinitionNode",
      "ClassStatementNode",
      "ClassMemberDefinitionStatement",
      "ClassMethodDefinitionStatement",
      "ClassSpecialMethodDefinitionStatement",
      "ClassInstantiationExpression",
      "PointerExpression",
      "DereferenceExpression",
      "WhileLoopStatement",
      "CastExpression",
      "ClassReadAccessorDefinitionStatement",
      "CharacterLiteralNode",
      "TypeAliasStatement",
      "SubscriptExpression",
      "RetrievalNode",
      "SuperClassFetch",
      "InstanceofExpression",
      "Generic",
      "ForLoopStatement",
      "RangedForLoopStatement",
      "UnaryOperation",
      "SizeofOperation",
      "FloatingPointLiteralNode",
      "StructureDefinitionNode",
      "ExportStatement",
      "VariableDeclarationStatement",
      "AliasStatement",
      "DeleteStatement",
      "ControlDirective",
      "TryCatchBlock",
      "ThrowStatement",
      "NullptrExpression",
      "CodeLiteralNode",
      "BitfieldDefinitionNode",
      "LambdaExpression",
    };
  };
};

#endif // ALTACORE_AST_SHARED_HPP
