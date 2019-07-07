#ifndef ALTACORE_AST_INTEGER_LITERAL_NODE_HPP
#define ALTACORE_AST_INTEGER_LITERAL_NODE_HPP

#include "literal-node.hpp"

namespace AltaCore {
  namespace AST {
    class IntegerLiteralNode: public LiteralNode {
      public:
        virtual const NodeType nodeType();

        uint64_t integer;

        static inline uint64_t parseInteger(std::string _raw) {
          uint64_t integer = 0;
          if (_raw.find_first_of("box") != _raw.npos) {
            auto idx = _raw.find_first_of("box");
            auto baseString = _raw.substr(0, idx);
            auto baseKind = _raw[idx];
            auto numberString = _raw.substr(idx + 1);
            auto base = std::stoull(baseString, nullptr, 10);
            if (base == 0) {
              if (baseKind == 'b') {
                base = 2;
              } else if (baseKind == 'o') {
                base = 8;
              } else if (baseKind == 'x') {
                base = 16;
              }
            } else if (base == 1) {
              throw std::runtime_error("can't have an integer with a base of 1");
            }
            integer = std::stoull(numberString, nullptr, base);
          } else {
            integer = std::stoull(_raw, nullptr, 10);
          }
          return integer;
        };

        IntegerLiteralNode(std::string raw);

        ALTACORE_AST_AUTO_DETAIL(IntegerLiteralNode);
    };
  };
};

#endif // ALTACORE_AST_INTEGER_LITERAL_NODE_HPP
