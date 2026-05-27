#pragma once
//2026-04-26 10:34

#include <optional>
#include <string_view>

#include "svg.h"
#include "map_renderer.h"
#include "transport_catalogue.h"

class RequestHandler {
  const transport_catalogue::TransportCatalogue& db_;
  const renderer::MapRenderer& renderer_;

public:
  RequestHandler(const transport_catalogue::TransportCatalogue& db,
    const renderer::MapRenderer& renderer);
  std::optional<BusInfo> GetBusStat(std::string_view bus_name) const;
  std::optional<transport_catalogue::BusesList> GetBusesByStop(std::string_view stop_name) const;
  svg::Document RenderMap() const;
};

