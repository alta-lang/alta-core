#include "../include/altacore.hpp"

#define AC_ATTRIBUTE_FUNC [](std::shared_ptr<AST::Node> _target, std::shared_ptr<DH::Node> _info, std::vector<Attributes::AttributeArgument> args) -> void
#define AC_ATTRIBUTE_CAST(x) auto target = std::dynamic_pointer_cast<AST::x>(_target);\
  auto info = std::dynamic_pointer_cast<DH::x>(_info);\
  if (!target || !info) throw std::runtime_error("this isn't supposed to happen");
#define AC_ATTRIBUTE(x, ...) Attributes::registerAttribute({ __VA_ARGS__ }, { AST::NodeType::x }, AC_ATTRIBUTE_FUNC {\
  AC_ATTRIBUTE_CAST(x);
#define AC_END_ATTRIBUTE })

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
};

#undef AC_END_ATTRIBUTE
#undef AC_ATTIBUTE
#undef AC_ATTRIBUTE_CAST
#undef AC_ATTRIBUTE_FUNC
