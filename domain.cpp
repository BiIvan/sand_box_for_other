//2026-04-26 10:35

#include <optional>

#include "domain.h"

constexpr std::string_view to_string(RequestType type) noexcept {
  switch (type) {
  case RequestType::Map:  return "Map";
  case RequestType::Bus:  return "Bus";
  case RequestType::Stop: return "Stop";
  }
  return "";
}

