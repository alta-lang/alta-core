#ifndef ALTACORE_DET_SHARED_HPP
#define ALTACORE_DET_SHARED_HPP

#include "shared.hpp"

namespace AltaCore {
  namespace DET {
    using Shared::Visibility;

    enum class NodeType {
      Node,
      Module,
      Scope,
      ScopeItem,
      Function,
      Type,
      Variable,
      Alias,
      Namespace,
      Class,
    };
    
    static const char* const NodeType_names[] = {
      "Node",
      "Module",
      "Scope",
      "ScopeItem",
      "Function",
      "Type",
      "Variable",
      "Alias",
      "Namespace",
      "Class",
    };

    enum class NativeType {
      Integer,
      Byte,
      Bool,
      Void,
      Double,
      Float,
      UserDefined,
    };

    static const char* const NativeType_names[] = {
      "Integer",
      "Byte",
      "Bool",
      "Void",
      "Double",
      "Float",
      "UserDefined",
    };


    // forward declarations:
    class Module;
    class Function;
    class Scope;
    class ScopeItem;
    class Namespace;
  };
};

#endif // ALTACORE_DET_SHARED_HPP
