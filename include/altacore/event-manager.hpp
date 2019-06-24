#ifndef ALTACORE_EVENT_MANAGER
#define ALTACORE_EVENT_MANAGER

#include <forward_list>
#include "optional.hpp"

//
// adapted from https://cpppatterns.com/patterns/apply-tuple-to-function.html
//
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
#include <functional>

namespace AltaCore {
  template<typename F, typename Tuple, size_t... S>
  decltype(auto) applyTupleImplementation(F&& fn, Tuple&& t, std::index_sequence<S...>) {
    return std::forward<F>(fn)(std::get<S>(std::forward<Tuple>(t))...);
  };

  template<typename F, typename Tuple>
  decltype(auto) applyTuple(F&& fn, Tuple&& t) {
    std::size_t constexpr tSize = std::tuple_size<typename std::remove_reference<Tuple>::type>::value;
    return applyTupleImplementation(
      std::forward<F>(fn),
      std::forward<Tuple>(t),
      std::make_index_sequence<tSize>()
    );
  };
};
//
//
//

namespace AltaCore {
  template<bool T>
  struct valueify;

  template<bool once = false, typename... Args>
  class EventManager {
    private:
      // we use a forward list because:
      //   * we don't care about the order in which we iterate
      //   * we don't need to access specific elements
      //   * we want the best possible insertion/deletion performance
      std::forward_list<std::function<void(Args...)>> callbacks;
      bool _dispatched = false;
      ALTACORE_OPTIONAL<std::tuple<Args...>> dispatchedArguments = ALTACORE_NULLOPT;

    public:
      void listen(const std::function<void(Args...)> callback) {
        if (valueify<once>::value && _dispatched) {
          return applyTuple(callback, dispatchedArguments.value());
        }
        callbacks.push_front(callback);
      };

      void dispatch(Args... args) {
        if (valueify<once>::value && _dispatched) return;
        _dispatched = true;
        dispatchedArguments = ALTACORE_MAKE_OPTIONAL(std::make_tuple(args...));
        for (const auto& callback: callbacks) {
          callback(args...);
        }
      };

      bool dispatched() const {
        return dispatched;
      };
  };
};

template<>
struct AltaCore::valueify<true> {
  static const bool value = true;
};

template<>
struct AltaCore::valueify<false> {
  static const bool value = false;
};

#endif // ALTACORE_EVENT_MANAGER
