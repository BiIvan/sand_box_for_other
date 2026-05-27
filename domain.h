#pragma once
//2026-04-26 10:35

#include <string>
#include <vector>
#include <cstddef>
#include <stdexcept>

#include "geo.h"

enum class RequestType {
  Map,
  Bus,
  Stop
};

struct ValidationError : std::runtime_error {
  using std::runtime_error::runtime_error;
};

struct Stop {
  std::string name_;
  geo::Coordinates coordinates{ 0.0, 0.0 };
};

struct Bus {
  std::string name_;
  std::vector<const Stop*> stops_;
  bool is_roundtrip_ = false;
};

struct BusInfo {
  std::size_t stop_count = 0;
  std::size_t unique_stop_count = 0;
  int road_length = 0;
  double geo_length = 0.0;
};