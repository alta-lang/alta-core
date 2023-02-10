#include "../../include/altacore/ast/bitfield-definition-node.hpp"
#include "../../include/altacore/util.hpp"

const AltaCore::AST::NodeType AltaCore::AST::BitfieldDefinitionNode::nodeType() {
  return NodeType::BitfieldDefinitionNode;
};

ALTACORE_AST_DETAIL_D(BitfieldDefinitionNode) {
  ALTACORE_MAKE_DH(BitfieldDefinitionNode);

  info->bitfield = DET::Class::create(name, info->inputScope, position, {}, true);
  info->inputScope->items.push_back(info->bitfield);
  info->bitfield->isStructure = true;
  info->bitfield->isBitfield = true;
  info->bitfield->isLiteral = false;
  info->isExport = std::find(modifiers.begin(), modifiers.end(), "export") != modifiers.end();

  if (info->isExport) {
    if (auto mod = Util::getModule(scope.get()).lock()) {
      mod->exports->items.push_back(info->bitfield);
    }
  }

  if (members.size() < 1) {
    ALTACORE_DETAILING_ERROR("bitfields must contain at least one member");
  }

  if (underlyingType) {
    info->underlyingType = underlyingType->fullDetail(info->bitfield->scope);
  } else {
    auto tmp = std::make_shared<Type>();
    auto length = std::get<3>(members.back()) + 1;
    if (length == 1) {
      tmp->_injected_type = std::make_shared<DET::Type>(DET::NativeType::Bool);
    } else if (length <= 8) {
      tmp->_injected_type = std::make_shared<DET::Type>(DET::NativeType::Byte, DET::Type::createModifierVector({ { DET::TypeModifierFlag::Unsigned } }));
    } else if (length <= 16) {
      tmp->_injected_type = std::make_shared<DET::Type>(DET::NativeType::Integer, DET::Type::createModifierVector({ { DET::TypeModifierFlag::Unsigned }, { DET::TypeModifierFlag::Short } }));
    } else if (length <= 32) {
      tmp->_injected_type = std::make_shared<DET::Type>(DET::NativeType::Integer, DET::Type::createModifierVector({ { DET::TypeModifierFlag::Unsigned } }));
    } else if (length <= 64) {
      tmp->_injected_type = std::make_shared<DET::Type>(DET::NativeType::Integer, DET::Type::createModifierVector({ { DET::TypeModifierFlag::Unsigned }, { DET::TypeModifierFlag::Long }, { DET::TypeModifierFlag::Long } }));
    } else {
      ALTACORE_DETAILING_ERROR("can't deduce underyling type for bitfield: size is greater than 64 bits");
    }
    info->underlyingType = tmp->fullDetail(info->bitfield->scope);
  }
  info->underlyingType->type->bitfield = info->bitfield;
  info->bitfield->underlyingBitfieldType = info->underlyingType->type;

  for (size_t i = 0; i < members.size(); i++) {
    auto [type, name, start, end] = members[i];
    std::shared_ptr<DH::Type> det = nullptr;
    if (type) {
      det = type->fullDetail(info->bitfield->scope);
    } else {
      auto tmp = std::make_shared<Type>();
      auto length = end - start + 1;
      if (length == 1) {
        tmp->_injected_type = std::make_shared<DET::Type>(DET::NativeType::Bool);
      } else if (length <= 8) {
        tmp->_injected_type = std::make_shared<DET::Type>(DET::NativeType::Byte, DET::Type::createModifierVector({ { DET::TypeModifierFlag::Unsigned } }));
      } else if (length <= 16) {
        tmp->_injected_type = std::make_shared<DET::Type>(DET::NativeType::Integer, DET::Type::createModifierVector({ { DET::TypeModifierFlag::Unsigned }, { DET::TypeModifierFlag::Short } }));
      } else if (length <= 32) {
        tmp->_injected_type = std::make_shared<DET::Type>(DET::NativeType::Integer, DET::Type::createModifierVector({ { DET::TypeModifierFlag::Unsigned } }));
      } else if (length <= 64) {
        tmp->_injected_type = std::make_shared<DET::Type>(DET::NativeType::Integer, DET::Type::createModifierVector({ { DET::TypeModifierFlag::Unsigned }, { DET::TypeModifierFlag::Long }, { DET::TypeModifierFlag::Long } }));
      } else {
        ALTACORE_DETAILING_ERROR("can't deduce type for bitfield field: size is greater than 64 bits");
      }
      det = tmp->fullDetail(info->bitfield->scope);
    }
    info->memberTypes.push_back(det);
    auto var = std::make_shared<DET::Variable>(name, det->type, position, info->bitfield->scope);
    var->isBitfieldEntry = true;
    var->bitfieldBits = std::make_pair(start, end);
    var->visibility = Visibility::Public;
    info->memberVariables.push_back(var);
    info->bitfield->scope->items.push_back(var);
  }

  for (auto& attr: attributes) {
    info->attributes.push_back(attr->fullDetail(info->inputScope, shared_from_this(), info));
  }

  return info;
};

ALTACORE_AST_VALIDATE_D(BitfieldDefinitionNode) {
  ALTACORE_VS_S(BitfieldDefinitionNode);

  if (underlyingType)
    underlyingType->validate(stack, info->underlyingType);

  for (size_t i = 0; i < members.size(); ++i) {
    auto& [type, name, start, end] = members[i];
    if (type)
      type->validate(stack, info->memberTypes[i]);
  }

  for (size_t i = 0; i < attributes.size(); ++i) {
    attributes[i]->validate(stack, info->attributes[i]);
  }

  ALTACORE_VS_E;
};
