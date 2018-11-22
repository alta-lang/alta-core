#ifndef ALTACORE_AST_SHARED_HPP
#define ALTACORE_AST_SHARED_HPP

#include <cinttypes>
#include "shared.hpp"

namespace AltaCore {
  namespace AST {
    using Shared::TypeModifierFlag;

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
    };

    enum class OperatorType {
      Addition,
      Subtraction,
      Multiplication,
      Division,
    };
  };
};

#endif // ALTACORE_AST_SHARED_HPP
