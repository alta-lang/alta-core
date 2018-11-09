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
    };
    
    static const char* const NodeType_names[] = {
      "Node",
      "Module",
      "Scope",
      "ScopeItem",
      "Function",
      "Type",
      "Variable",
    };

    enum class NativeType {
      Integer,
      Byte,
    };

    static const char* const NativeType_names[] = {
      "Integer",
      "Byte",
    };


    // forward declarations:
    class Module;
    class Function;
    class Scope;
    class ScopeItem;
  };
};

#endif // ALTACORE_DET_SHARED_HPP
