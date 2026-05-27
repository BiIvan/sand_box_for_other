#pragma once
//2026-04-26 10:34

#include <deque>
#include <string>
#include <vector>
#include <cstddef>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include "geo.h"
#include "domain.h"

namespace transport_catalogue {

  using BusesList = std::unordered_set<std::string_view>;
  using OptionalBusesList = std::optional<transport_catalogue::BusesList>;

  class TransportCatalogue {

    struct StopPair {
      const Stop* from = nullptr;
      const Stop* to = nullptr;
      bool operator==(const StopPair& other) const {
        return from == other.from && to == other.to;
      }
    };

    struct StopPairHasher {
      std::size_t operator()(const StopPair& pair) const {
        return std::hash<const void*>{}(pair.from) * 37u
          + std::hash<const void*>{}(pair.to);
      }
    };

    int ComputeStopRouteLength(const Bus& bus) const;
    int ComputeRoadRouteLength(const Bus& bus) const;
    double ComputeGeoRouteLength(const Bus& bus) const;
    std::size_t ComputeUniqueStopCount(const Bus& bus) const;
    int GetDistanceBetweenStops(const Stop* from, const Stop* to) const;

    std::deque<Stop> stops_;
    std::deque<Bus> buses_;
    std::unordered_map<StopPair, int, StopPairHasher> distances_;
    std::unordered_map<std::string_view, const Bus*> buses_by_name_;
    std::unordered_map<std::string_view, const Stop*> stops_by_name_;
    std::unordered_map<const Stop*, BusesList> buses_by_stop_;

  public:
    void AddStop(const std::string& name, geo::Coordinates coordinates);
    void AddBus(const std::string& name, const std::vector<const Stop*>& stops, bool is_roundtrip);
    void SetDistance(const Stop* from, const Stop* to, int distance);
    const Stop* FindStop(std::string_view name) const;
    const Bus* FindBus(std::string_view name) const;
    std::optional<BusInfo> GetBusInfo(std::string_view bus_name) const;
    BusesList GetStopInfo(std::string_view stop_name) const;
    const std::deque<Bus> GetAllBuses() const;
  };

}  // namespace transport_catalogue