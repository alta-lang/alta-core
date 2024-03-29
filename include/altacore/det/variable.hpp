#ifndef ALTACORE_DET_VARIABLE_HPP
#define ALTACORE_DET_VARIABLE_HPP

#include "scope-item.hpp"
#include "type.hpp"
#include <string>

namespace AltaCore {
  namespace DET {
    class Variable: public ScopeItem {
      public:
        virtual const NodeType nodeType();
        virtual std::shared_ptr<Node> clone();
        virtual std::shared_ptr<Node> deepClone();

        std::shared_ptr<Type> type;
        bool isLiteral = false;
        bool isExport = false;
        bool isVariable = false;
        bool isBitfieldEntry = false;
        std::pair<size_t, size_t> bitfieldBits;

        Variable(std::string name, std::shared_ptr<Type> type, AltaCore::Errors::Position position, std::shared_ptr<Scope> parentScope = nullptr);

        virtual std::string toString() const;
    };
  };
};

#endif // ALTACORE_DET_VARIABLE_HPP
