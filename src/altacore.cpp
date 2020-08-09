#include "../include/altacore.hpp"
#include <sstream>
#include <crossguid/guid.hpp>

#define AC_ATTRIBUTE_FUNC [](std::shared_ptr<AST::Node> _target, std::shared_ptr<DH::Node> _info, std::vector<Attributes::AttributeArgument> args) -> void
#define AC_ATTRIBUTE_CAST(x) auto target = std::dynamic_pointer_cast<AST::x>(_target);\
  auto info = std::dynamic_pointer_cast<DH::x>(_info);\
  if (!target || !info) throw std::runtime_error("this isn't supposed to happen");
#define AC_ATTRIBUTE(x, ...) Attributes::registerAttribute({ __VA_ARGS__ }, { AST::NodeType::x }, AC_ATTRIBUTE_FUNC {\
  AC_ATTRIBUTE_CAST(x);
#define AC_GENERAL_ATTRIBUTE(...) Attributes::registerAttribute({ __VA_ARGS__ }, {}, AC_ATTRIBUTE_FUNC {
#define AC_END_ATTRIBUTE })
#define AC_END_ATTRIBUTE_OPTIONS(...) }, "", __VA_ARGS__)

void AltaCore::registerGlobalAttributes() {
  AC_ATTRIBUTE(FunctionDefinitionNode, "read");
    info->function->isAccessor = true;
  AC_END_ATTRIBUTE;

  AC_ATTRIBUTE(ClassSpecialMethodDefinitionStatement, "copy");
    info->isCopyConstructor = true;
  AC_END_ATTRIBUTE;

  AC_ATTRIBUTE(StructureDefinitionStatement, "external");
    target->isExternal = info->isExternal = info->structure->isExternal = true;
  AC_END_ATTRIBUTE;

  AC_ATTRIBUTE(StructureDefinitionStatement, "typed");
    target->isTyped = info->isTyped = info->structure->isTyped = true;
  AC_END_ATTRIBUTE;

  AC_ATTRIBUTE(AssignmentExpression, "strict");
    info->strict = true;
  AC_END_ATTRIBUTE;

  AC_ATTRIBUTE(FunctionDefinitionNode, "throwing");
    info->function->throws(true);
  AC_END_ATTRIBUTE;

  AC_GENERAL_ATTRIBUTE("noruntime");
    if (auto target = std::dynamic_pointer_cast<AST::RootNode>(_target)) {
      auto info = std::dynamic_pointer_cast<DH::RootNode>(_info);
      info->module->noRuntimeInclude = true;
      info->module->scope->noRuntime = true;
    } else if (auto target = std::dynamic_pointer_cast<AST::FunctionDefinitionNode>(_target)) {
      auto info = std::dynamic_pointer_cast<DH::FunctionDefinitionNode>(_info);
      info->function->scope->noRuntime = true;
    } else {
      throw std::runtime_error("noruntime attribute applied to an invalid node");
    }
  AC_END_ATTRIBUTE;

  AC_ATTRIBUTE(LambdaExpression, "copy");
    for (auto& arg: args) {
      if (!arg.isScopeItem) {
        throw std::runtime_error("invalid argument for lambda expression @copy attribute");
      }
      if (auto var = std::dynamic_pointer_cast<DET::Variable>(arg.item)) {
        info->toCopy.push_back(var);
      } else {
        throw std::runtime_error("non-variable argument provided to lambda expression @copy attribute");
      }
    }
  AC_END_ATTRIBUTE;

  AC_ATTRIBUTE(ClassSpecialMethodDefinitionStatement, "from");
    info->isCastConstructor = true;
  AC_END_ATTRIBUTE;

  AC_ATTRIBUTE(FunctionDefinitionNode, "virtual");
    info->function->_virtual = true;
  AC_END_ATTRIBUTE;

  AC_ATTRIBUTE(FunctionDefinitionNode, "override");
    bool isVirt = info->function->_virtual;
    info->function->_virtual = false;

    if (!info->function->isVirtual()) {
      throw std::runtime_error("`override` method doesn't override any parent methods (maybe an incorrect signature?)");
    }

    info->function->_virtual = isVirt;
  AC_END_ATTRIBUTE_OPTIONS(true);

  AC_ATTRIBUTE(ClassDefinitionNode, "copy");
    for (auto& arg: args) {
      if (!arg.isScopeItem) {
        throw std::runtime_error("invalid argument for capture class @copy attribute");
      }
      if (auto var = std::dynamic_pointer_cast<DET::Variable>(arg.item)) {
        info->toCopy.push_back(var);
      } else {
        throw std::runtime_error("non-variable argument provided to capture class @copy attribute");
      }
    }
  AC_END_ATTRIBUTE;

  AC_ATTRIBUTE(Accessor, "returnTypeOf");
    info->fetchingReturnType = true;
  AC_END_ATTRIBUTE;

  AC_ATTRIBUTE(Fetch, "returnTypeOf");
    info->fetchingReturnType = true;
  AC_END_ATTRIBUTE;

  AC_ATTRIBUTE(SubscriptExpression, "reverse");
    info->reverseLookup = true;
  AC_END_ATTRIBUTE;
};

AltaCore::DetailHandles::Node::Node(decltype(AltaCore::DetailHandles::Node::inputScope) _inputScope):
  inputScope(_inputScope)
{
  std::stringstream uuidStream;
  uuidStream << xg::newGuid();
  id = uuidStream.str();
};

#undef AC_END_ATTRIBUTE
#undef AC_ATTIBUTE
#undef AC_ATTRIBUTE_CAST
#undef AC_ATTRIBUTE_FUNC
