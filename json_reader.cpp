//2026.05.27_23:20

#include <string>
#include <vector>
#include <utility>
#include <cstdint>
#include <algorithm>
#include <stdexcept>

#include "domain.h"
#include "json_reader.h"
#include "json_builder.h"
#include "map_renderer.h"
#include "request_handler.h"
#include "transport_catalogue.h"

namespace json_reader {

  namespace {

    struct ParsedStop {
      std::string name;
      geo::Coordinates coordinates;
      std::vector<std::pair<std::string, int>> distances;
    };

    struct ParsedBus {
      std::string name;
      std::vector<std::string> stops;
      bool is_roundtrip = false;
    };

    struct ParsedStatRequest {
      std::uint64_t id = 0;
      RequestType type = RequestType::Bus;
      std::string name = "";
    };

    json::Node NotFound(const ParsedStatRequest& request) {
      return
        json::Builder{}
        .StartDict()
        .Key("request_id").Value(json::Node(static_cast<int>(request.id)))
        .Key("error_message")
        .Value(json::Node(std::string("not found")))
        .EndDict()
        .Build();
    }

    json::Array SortedBusesArray(transport_catalogue::OptionalBusesList buses) {
      std::vector<std::string> sorted_buses;
      sorted_buses.reserve(buses->size());
      for (std::string_view bus_name : *buses) {
        sorted_buses.emplace_back(bus_name);
      }
      std::sort(sorted_buses.begin(), sorted_buses.end());
      json::Array buses_array;
      for (const std::string& bus_name : sorted_buses) {
        buses_array.emplace_back(bus_name);
      }
      return buses_array;
    }

    RequestType ParseRequestType(const std::string& type) {
      if (type == "Map") {
        return RequestType::Map;
      }
      if (type == "Bus") {
        return RequestType::Bus;
      }
      if (type == "Stop") {
        return RequestType::Stop;
      }
      throw ValidationError("Unknown request type: " + type);
    }

    std::string ParseColorToString(const json::Node& node) {
      if (node.IsString()) {
        return node.AsString();
      }
      if (node.IsArray()) {
        const auto& arr = node.AsArray();
        if (arr.size() == 3) {
          return "rgb(" + std::to_string(arr[0].AsInt()) + "," +
            std::to_string(arr[1].AsInt()) + "," +
            std::to_string(arr[2].AsInt()) + ")";
        }
        if (arr.size() == 4) {
          return "rgba(" + std::to_string(arr[0].AsInt()) + "," +
            std::to_string(arr[1].AsInt()) + "," +
            std::to_string(arr[2].AsInt()) + "," +
            std::to_string(arr[3].AsDouble()) + ")";
        }
      }
      throw ValidationError("Invalid color format");
    }

    renderer::RenderSettings ParseRender(const json::Dict& render_request) {
      renderer::RenderSettings result;
      result.width = render_request.at("width").AsDouble();
      result.height = render_request.at("height").AsDouble();
      result.padding = render_request.at("padding").AsDouble();
      result.line_width = render_request.at("line_width").AsDouble();
      result.stop_radius = render_request.at("stop_radius").AsDouble();
      result.bus_label_font_size = render_request.at("bus_label_font_size").AsInt();
      {
        const auto& arr = render_request.at("bus_label_offset").AsArray();
        result.bus_label_offset = { arr[0].AsDouble(), arr[1].AsDouble() };
      }
      result.stop_label_font_size = render_request.at("stop_label_font_size").AsInt();
      {
        const auto& arr = render_request.at("stop_label_offset").AsArray();
        result.stop_label_offset = { arr[0].AsDouble(), arr[1].AsDouble() };
      }
      result.underlayer_color = ParseColorToString(render_request.at("underlayer_color"));
      result.underlayer_width = render_request.at("underlayer_width").AsDouble();
      for (const auto& color_node : render_request.at("color_palette").AsArray()) {
        result.color_palette.push_back(ParseColorToString(color_node));
      }
      if (result.color_palette.empty()) {
        result.color_palette.push_back(std::string("black"));
      }
      return result;
    }

    std::vector<ParsedStop> ParseStops(const json::Array& base_requests) {
      std::vector<ParsedStop> result;
      for (const json::Node& node : base_requests) {
        const json::Dict& dict = node.AsDict();
        if (!dict.count("type") || dict.at("type").AsString() != "Stop") {
          continue;
        }
        ParsedStop stop;
        stop.name = dict.at("name").AsString();
        stop.coordinates = {
            dict.at("latitude").AsDouble(),
            dict.at("longitude").AsDouble()
        };
        if (const auto it = dict.find("road_distances"); it != dict.end()) {
          for (const auto& [to_stop, dist_node] : it->second.AsDict()) {
            stop.distances.emplace_back(to_stop, dist_node.AsInt());
          }
        }
        result.push_back(std::move(stop));
      }
      return result;
    }

    std::vector<ParsedBus> ParseBuses(const json::Array& base_requests) {
      std::vector<ParsedBus> result;
      for (const json::Node& node : base_requests) {
        const json::Dict& dict = node.AsDict();
        if (!dict.count("type") || dict.at("type").AsString() != "Bus") {
          continue;
        }
        ParsedBus bus;
        bus.name = dict.at("name").AsString();
        bus.is_roundtrip = dict.at("is_roundtrip").AsBool();
        for (const json::Node& stop_node : dict.at("stops").AsArray()) {
          bus.stops.push_back(stop_node.AsString());
        }
        result.push_back(std::move(bus));
      }
      return result;
    }

    std::vector<ParsedStatRequest> ParseStatRequests(const json::Array& stat_requests) {
      std::vector<ParsedStatRequest> result;
      for (const json::Node& node : stat_requests) {
        const json::Dict& dict = node.AsDict();
        result.push_back({
            static_cast<std::uint64_t>(dict.at("id").AsInt()),
            ParseRequestType(dict.at("type").AsString()),
            ParseRequestType(dict.at("type").AsString()) != RequestType::Map ? dict.at("name").AsString() : std::string("Map")
          });
      }
      return result;
    }


    void FillCatalogue(transport_catalogue::TransportCatalogue& catalogue,
      const std::vector<ParsedStop>& stops,
      const std::vector<ParsedBus>& buses) {
      for (const ParsedStop& stop : stops) {
        catalogue.AddStop(stop.name, stop.coordinates);
      }
      for (const ParsedStop& stop : stops) {
        const Stop* from = catalogue.FindStop(stop.name);
        for (const auto& [to_name, distance] : stop.distances) {
          const Stop* to = catalogue.FindStop(to_name);
          catalogue.SetDistance(from, to, distance);
        }
      }
      for (const ParsedBus& bus : buses) {
        std::vector<const Stop*> route;
        route.reserve(bus.stops.size());
        for (const std::string& stop_name : bus.stops) {
          route.push_back(catalogue.FindStop(stop_name));
        }
        catalogue.AddBus(bus.name, route, bus.is_roundtrip);
      }
    }

    json::Node MakeBusResponse(const ParsedStatRequest& request,
      const RequestHandler& handler) {
      const auto info = handler.GetBusStat(request.name);
      if (!info) {
        return NotFound(request);
      }
      else {
        return
          json::Builder{}
          .StartDict()
          .Key("request_id").Value(json::Node(static_cast<int>(request.id)))
          .Key("stop_count").Value(json::Node(static_cast<int>(info->stop_count)))
          .Key("unique_stop_count")
          .Value(json::Node(static_cast<int>(info->unique_stop_count)))
          .Key("route_length").Value(json::Node(info->road_length))
          .Key("curvature").Value(json::Node(info->geo_length > 0.0
            ? static_cast<double>(info->road_length) / info->geo_length
            : 0))
          .EndDict()
          .Build();
      }
    }

    json::Node MakeMapResponse(const ParsedStatRequest& request, std::string map_str) {
      return
        json::Builder{}
        .StartDict()
        .Key("request_id").Value(json::Node(static_cast<int>(request.id)))
        .Key("map").Value(json::Node(std::move(map_str)))
        .EndDict()
        .Build();
    }

    json::Node MakeStopResponse(const ParsedStatRequest& request,
      const RequestHandler& handler) {
      const auto buses = handler.GetBusesByStop(request.name);
      if (!buses) {
        return NotFound(request);
      }
      return
        json::Builder{}
        .StartDict()
        .Key("request_id").Value(json::Node(static_cast<int>(request.id)))
        .Key("buses").Value(SortedBusesArray(buses))
        .EndDict()
        .Build();
    }

    json::Document BuildResponse(const RequestHandler& handler,
      const std::vector<ParsedStatRequest>& stat_requests,
      std::string map_str = "")
    {
      auto responses = json::Builder{}.StartArray();

      for (const ParsedStatRequest& request : stat_requests) {
        if (request.type == RequestType::Bus) {
          responses = std::move(responses).Value(MakeBusResponse(request, handler));
        }
        else if (request.type == RequestType::Stop) {
          responses = std::move(responses).Value(MakeStopResponse(request, handler));
        }
        else {
          responses = std::move(responses).Value(MakeMapResponse(request, map_str));
        }
      }
      return json::Document(std::move(responses).EndArray().Build());
    }
  }  // namespace

  json::Document ProcessRequests(const json::Document& input, std::string map_str) {
    const json::Dict& root = input.GetRoot().AsDict();
    const json::Array empty_array;
    const json::Dict empty_dict;
    const json::Array& base_requests =
      root.count("base_requests") ? root.at("base_requests").AsArray() : empty_array;
    const json::Dict& render_requests =
      root.count("render_settings") ? root.at("render_settings").AsDict() : empty_dict;
    const json::Array& stat_requests =
      root.count("stat_requests") ? root.at("stat_requests").AsArray() : empty_array;
    const std::vector<ParsedStop> stops = ParseStops(base_requests);
    const std::vector<ParsedBus> buses = ParseBuses(base_requests);
    const renderer::RenderSettings render_settings = ParseRender(render_requests);
    const std::vector<ParsedStatRequest> stats = ParseStatRequests(stat_requests);
    transport_catalogue::TransportCatalogue catalogue;
    FillCatalogue(catalogue, stops, buses);
    RequestHandler handler(catalogue, renderer::MapRenderer(render_settings));
    return BuildResponse(handler, stats, map_str);
  }
}  // namespace json_reader