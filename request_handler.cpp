//2026-04-26 10:33

#include "request_handler.h"

RequestHandler::RequestHandler(const transport_catalogue::TransportCatalogue& db,
  const renderer::MapRenderer& renderer)
  : db_(db)
  , renderer_(renderer) {
}

std::optional<BusInfo> RequestHandler::GetBusStat(std::string_view bus_name) const {
  return db_.GetBusInfo(bus_name);
}

std::optional<transport_catalogue::BusesList> RequestHandler::GetBusesByStop(std::string_view stop_name) const {
  if (db_.FindStop(stop_name) == nullptr) {
    return std::nullopt;
  }
  return db_.GetStopInfo(stop_name);
}

svg::Document RequestHandler::RenderMap() const {
  return {};
}
 