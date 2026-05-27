//2026-04-26 10:34

#include <cmath>
#include <algorithm>
#include <unordered_set>

#include "transport_catalogue.h"

using namespace transport_catalogue;

void TransportCatalogue::AddStop(const std::string& name, geo::Coordinates coordinates) {
  if (FindStop(name) != nullptr) {
    return;
  }
  stops_.push_back({ name, coordinates });
  const Stop* stop = &stops_.back();
  stops_by_name_[stop->name_] = stop;
  buses_by_stop_[stop];
}

void TransportCatalogue::AddBus(const std::string& name,
  const std::vector<const Stop*>& stops,
  bool is_roundtrip) {
  if (FindBus(name) != nullptr) {
    return;
  }
  buses_.push_back({ name, stops, is_roundtrip });
  const Bus* bus = &buses_.back();
  buses_by_name_[bus->name_] = bus;

  for (const Stop* stop : bus->stops_) {
    if (stop != nullptr) {
      buses_by_stop_[stop].insert(bus->name_);
    }
  }
}

void TransportCatalogue::SetDistance(const Stop* from, const Stop* to, int distance) {
  if (from == nullptr || to == nullptr) {
    return;
  }
  distances_[{from, to}] = distance;
}

const Stop* TransportCatalogue::FindStop(std::string_view name) const {
  const auto it = stops_by_name_.find(name);
  if (it == stops_by_name_.end()) {
    return nullptr;
  }
  return it->second;
}

const Bus* TransportCatalogue::FindBus(std::string_view name) const {
  const auto it = buses_by_name_.find(name);
  if (it == buses_by_name_.end()) {
    return nullptr;
  }
  return it->second;
}

std::optional<BusInfo> TransportCatalogue::GetBusInfo(std::string_view bus_name) const {
  const Bus* bus = FindBus(bus_name);
  if (bus == nullptr) {
    return std::nullopt;
  }
  const int road_length = ComputeRoadRouteLength(*bus);
  const double geo_length = ComputeGeoRouteLength(*bus);
  BusInfo info;
  info.unique_stop_count = ComputeUniqueStopCount(*bus);
  if (bus->is_roundtrip_) {
    info.stop_count = static_cast<int>(bus->stops_.size());
  }
  else if (bus->stops_.empty()) {
    info.stop_count = 0;
  }
  else {
    info.stop_count = static_cast<int>(bus->stops_.size() * 2 - 1);
  }
  info.road_length = road_length;
  info.geo_length = geo_length;
  return info;
}

BusesList TransportCatalogue::GetStopInfo(std::string_view stop_name) const {
  const Stop* stop = FindStop(stop_name);
  if (stop == nullptr) {
    return {};
  }
  const auto it = buses_by_stop_.find(stop);
  if (it != buses_by_stop_.end()) {
    return it->second;
  }
  return {};
}

int TransportCatalogue::ComputeStopRouteLength(const Bus& bus) const {
  if (bus.stops_.size() < 2) {
    return static_cast<int>(bus.stops_.size());
  }
  if (bus.is_roundtrip_) {
    return static_cast<int>(bus.stops_.size());
  }
  return static_cast<int>(bus.stops_.size() * 2 - 1);
}

int TransportCatalogue::ComputeRoadRouteLength(const Bus& bus) const {
  int len = 0;
  for (size_t i = 1; i < bus.stops_.size(); ++i) {
    int d = GetDistanceBetweenStops(bus.stops_[i - 1], bus.stops_[i]);
    if (d == 0) {
      d = GetDistanceBetweenStops(bus.stops_[i], bus.stops_[i - 1]);
    }
    len += d;
  }
  if (!bus.is_roundtrip_) {
    for (size_t i = bus.stops_.size(); i > 1; --i) {
      int d = GetDistanceBetweenStops(bus.stops_[i - 1], bus.stops_[i - 2]);
      if (d == 0) {
        d = GetDistanceBetweenStops(bus.stops_[i - 2], bus.stops_[i - 1]);
      }
      len += d;
    }
  }
  return len;
}

double TransportCatalogue::ComputeGeoRouteLength(const Bus& bus) const {
  if (bus.stops_.size() < 2) {
    return 0.0;
  }
  double route_length = 0.0;
  for (size_t i = 1; i < bus.stops_.size(); ++i) {
    route_length += geo::ComputeDistance(bus.stops_[i - 1]->coordinates,
      bus.stops_[i]->coordinates);
  }
  if (!bus.is_roundtrip_) {
    route_length *= 2.0;
  }
  return route_length;
}

std::size_t TransportCatalogue::ComputeUniqueStopCount(const Bus& bus) const {
  std::unordered_set<const Stop*> unique_stops;
  unique_stops.reserve(bus.stops_.size());
  for (const Stop* stop : bus.stops_) {
    if (stop != nullptr) {
      unique_stops.insert(stop);
    }
  }
  return unique_stops.size();
}

int TransportCatalogue::GetDistanceBetweenStops(const Stop* from, const Stop* to) const {
  if (from == nullptr || to == nullptr) {
    return 0;
  }
  if (const auto it = distances_.find({ from, to }); it != distances_.end()) {
    return it->second;
  }
  return 0;
}

const std::deque<Bus> TransportCatalogue::GetAllBuses() const {
  std::deque<Bus> b_tmp = buses_;
  std::sort(b_tmp.begin(), b_tmp.end(), 
    [&](const Bus& one, const Bus& two) { return one.name_ < two.name_; }
  );
  return b_tmp;
}