#include "../../include/altacore/ast/attribute-node.hpp"
#include "../../include/altacore/util.hpp"
#include "../../include/altacore/ast/literal-node.hpp"

const AltaCore::AST::NodeType AltaCore::AST::AttributeNode::nodeType() {
  return NodeType::AttributeNode;
};

AltaCore::AST::AttributeNode::AttributeNode(std::vector<std::string> _accessors, std::vector<std::shared_ptr<AltaCore::AST::Node>> _arguments):
  accessors(_accessors),
  arguments(_arguments)
  {};

bool AltaCore::AST::AttributeNode::matches(std::vector<std::string> path) {
  if (accessors.size() != path.size()) return false;
  for (size_t i = 0; i < path.size(); i++) {
    if (accessors[i] != path[i]) return false;
  }
  return true;
};

std::shared_ptr<AltaCore::DH::Node> AltaCore::AST::AttributeNode::detail(std::shared_ptr<AltaCore::DET::Scope> scope, std::shared_ptr<Node> target, std::shared_ptr<DH::Node> targetInfo) {
  ALTACORE_MAKE_DH(AttributeNode);

  info->target = target;
  info->targetInfo = targetInfo;

  info->module = Util::getModule(scope.get());

  for (auto& arg: arguments) {
    auto argDet = (arg->nodeType() == NodeType::Type) ? std::dynamic_pointer_cast<Type>(arg)->fullDetail(scope) : arg->fullDetail(scope);
    info->arguments.push_back(argDet);
    if (arg->nodeType() == NodeType::StringLiteralNode) {
      info->attributeArguments.emplace_back(std::dynamic_pointer_cast<AST::StringLiteralNode>(arg)->value);
    } else if (arg->nodeType() == NodeType::IntegerLiteralNode) {
      info->attributeArguments.emplace_back(std::stoi(std::dynamic_pointer_cast<AST::IntegerLiteralNode>(arg)->raw));
    } else if (arg->nodeType() == NodeType::BooleanLiteralNode) {
      info->attributeArguments.emplace_back(std::dynamic_pointer_cast<AST::BooleanLiteralNode>(arg)->value);
    } else if (arg->nodeType() == NodeType::FloatingPointLiteralNode) {
      info->attributeArguments.emplace_back(std::stod(std::dynamic_pointer_cast<AST::FloatingPointLiteralNode>(arg)->raw));
    } else if (arg->nodeType() == NodeType::CharacterLiteralNode) {
      info->attributeArguments.emplace_back((int)std::dynamic_pointer_cast<AST::CharacterLiteralNode>(arg)->value);
    } else if (arg->nodeType() == NodeType::Fetch) {
      auto fetch = std::dynamic_pointer_cast<AST::Fetch>(arg);
      auto det = std::dynamic_pointer_cast<DH::Fetch>(argDet);
      if (det->items.size() == 0) {
        ALTACORE_DETAILING_ERROR("no items found in retrieval node argument for attribute");
      }
      if (!det->narrowedTo) {
        fetch->narrowTo(det, 0);
        // TODO: warn about retrieval being automatically narrowed
      }
      info->attributeArguments.emplace_back(det->narrowedTo);
    } else if (arg->nodeType() == NodeType::Accessor) {
      auto acc = std::dynamic_pointer_cast<AST::Accessor>(arg);
      auto det = std::dynamic_pointer_cast<DH::Accessor>(argDet);
      if (det->items.size() == 0) {
        ALTACORE_DETAILING_ERROR("no items found in retrieval node argument for attribute");
      }
      if (!det->narrowedTo) {
        acc->narrowTo(det, 0);
        // TODO: warn about retrieval being automatically narrowed
      }
      info->attributeArguments.emplace_back(det->narrowedTo);
    } else if (arg->nodeType() == NodeType::Type) {
      auto type = std::dynamic_pointer_cast<DH::Type>(argDet);
      info->attributeArguments.emplace_back(type->type);
    } else {
      throw std::runtime_error("welp.");
    }
  }

  findAttribute(info);
  run(info, info->target, info->targetInfo);

  return info;
};

void AltaCore::AST::AttributeNode::run(std::shared_ptr<DH::AttributeNode> info, std::shared_ptr<AltaCore::AST::Node> target, std::shared_ptr<AltaCore::DH::Node> tgtInfo) {
  if (info->attribute) {
    auto attr = *info->attribute;
    if (attr.callback) {
      attr.callback(target ? target : info->module.lock()->ast.lock(), tgtInfo ? tgtInfo : info->module.lock()->ast.lock()->info, info->attributeArguments);
    }
  }
};

void AltaCore::AST::AttributeNode::findAttribute(std::shared_ptr<DH::AttributeNode> info) {
  info->attribute = Attributes::findAttribute(
    accessors,
    info->target ? ALTACORE_MAKE_OPTIONAL(info->target->nodeType()) : ALTACORE_NULLOPT,
    info->module.lock()->path.toString()
  );
};

std::string AltaCore::AST::AttributeNode::id() const {
  std::string result = "";

  bool isFirst = true;
  for (auto& item: accessors) {
    if (isFirst) {
      isFirst = false;
    } else {
      result += '.';
    }
    result += item;
  }

  return result;
};

ALTACORE_AST_VALIDATE_D(AttributeNode) {
  ALTACORE_VS_S(AttributeNode);
  for (auto acc: accessors) {
    if (acc.empty()) {
      ALTACORE_VALIDATION_ERROR("attribute node accessor component can't be empty");
    }
  }

  for (size_t i = 0; i < arguments.size(); i++) {
    auto& arg = arguments[i];
    auto& argDet = info->arguments[i];
    if (!arg) ALTACORE_VALIDATION_ERROR("empty argument in attribute node");
    arg->validate(stack, argDet);
  }
  ALTACORE_VS_E;
};
