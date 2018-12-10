#ifndef ALTACORE_DET_SHARED_HPP
#define ALTACORE_DET_SHARED_HPP

namespace AltaCore {
  namespace DET {
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
    };

    enum class NativeType {
      Integer,
      Byte,
      Bool,
      Void,
    };

    static const char* const NativeType_names[] = {
      "Integer",
      "Byte",
      "Bool",
      "Void",
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
