#include "../include/altacore/logging.hpp"

namespace AltaCore {
  namespace Logging {
    std::vector<Listener> listeners;
  };
};

size_t AltaCore::Logging::registerListener(Listener listener) {
  const auto idx = listeners.size();
  listeners.push_back(listener);
  return idx;
};

void AltaCore::Logging::log(Message message) {
  for (auto& listener: listeners) {
    listener(message);
  }
};
