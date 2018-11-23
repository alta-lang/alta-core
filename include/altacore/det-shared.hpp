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
    };

    enum class NativeType {
      Integer,
      Byte,
      Bool,
    };

    static const char* const NativeType_names[] = {
      "Integer",
      "Byte",
      "Bool",
    };


    // forward declarations:
    class Module;
    class Function;
    class Scope;
    class ScopeItem;
  };
};

#endif // ALTACORE_DET_SHARED_HPP
