//2026-04-26 10:33

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <exception>
#include <stdexcept>

#include "svg.h"
#include "json_reader.h"
#include "json_builder.h"
#include "map_renderer.h"
#include "transport_catalogue.h"

using json::Array;
using json::Dict;
using json::Document;
using transport_catalogue::TransportCatalogue;

namespace {
  void ThrowParsingError(const std::string& message) {
    throw std::runtime_error(message);
  }
  const Dict& GetRootDict(const Document& doc) {
    return doc.GetRoot().AsDict();
  }
  const Array& GetBaseRequests(const Dict& root) {
    if (!root.count("base_requests")) {
      ThrowParsingError("Input error: field 'base_requests' is missing");
    }
    return root.at("base_requests").AsArray();
  }
  const Dict& GetRenderSettingsNode(const Dict& root) {
    if (!root.count("render_settings")) {
      ThrowParsingError("Input error: field 'render_settings' is missing");
    }
    return root.at("render_settings").AsDict();
  }
  void ParseStops(const Array& base_requests, TransportCatalogue& catalogue) {
    for (const auto& request_node : base_requests) {
      const Dict& request = request_node.AsDict();
      if (!request.count("type") || request.at("type").AsString() != "Stop") {
        continue;
      }
      if (!request.count("name") ||
        !request.count("latitude") ||
        !request.count("longitude")) {
        ThrowParsingError("Input error: Stop request is missing required fields");
      }
      catalogue.AddStop(
        request.at("name").AsString(),
        {
            request.at("latitude").AsDouble(),
            request.at("longitude").AsDouble()
        }
      );
    }
  }
  void ParseDistances(const Array& base_requests, TransportCatalogue& catalogue) {
    for (const auto& request_node : base_requests) {
      const Dict& request = request_node.AsDict();
      if (!request.count("type") || request.at("type").AsString() != "Stop") {
        continue;
      }
      if (!request.count("name")) {
        ThrowParsingError("Input error: Stop request without name");
      }
      const std::string& from_stop_name = request.at("name").AsString();
      const Stop* from_stop = catalogue.FindStop(from_stop_name);
      if (!from_stop) {
        ThrowParsingError("Catalogue error: stop not found: " + from_stop_name);
      }
      if (!request.count("road_distances")) {
        continue;
      }
      const Dict& distances = request.at("road_distances").AsDict();
      for (const auto& [to_stop_name, distance_node] : distances) {
        const Stop* to_stop = catalogue.FindStop(to_stop_name);
        if (!to_stop) {
          ThrowParsingError(
            "Input error: road_distances refers to unknown stop: " + to_stop_name
          );
        }
        catalogue.SetDistance(from_stop, to_stop, distance_node.AsInt());
      }
    }
  }
  void ParseBuses(const Array& base_requests, TransportCatalogue& catalogue) {
    for (const auto& request_node : base_requests) {
      const Dict& request = request_node.AsDict();
      if (!request.count("type") || request.at("type").AsString() != "Bus") {
        continue;
      }
      if (!request.count("name") ||
        !request.count("stops") ||
        !request.count("is_roundtrip")) {
        ThrowParsingError("Input error: Bus request is missing required fields");
      }
      const std::string& bus_name = request.at("name").AsString();
      const Array& stops_array = request.at("stops").AsArray();
      const bool is_roundtrip = request.at("is_roundtrip").AsBool();
      std::vector<const Stop*> bus_stops;
      bus_stops.reserve(stops_array.size());
      for (const auto& stop_node : stops_array) {
        const std::string& stop_name = stop_node.AsString();
        const Stop* stop = catalogue.FindStop(stop_name);
        if (!stop) {
          ThrowParsingError(
            "Input error: bus '" + bus_name + "' refers to unknown stop: " + stop_name
          );
        }
        bus_stops.push_back(stop);
      }
      catalogue.AddBus(bus_name, bus_stops, is_roundtrip);
    }
  }
}  // namespace

int main() {
  json::Print(
    json::Document{
        json::Builder{}
        .StartDict()
            .Key("key1").Value(123)
            .Key("key2").Value("value2")
            .Key("key3").StartArray()
                .Value(456)
                .StartDict().EndDict()
                .StartDict()
                    .Key("")
                    .Value(nullptr)
                .EndDict()
                .Value("")
            .EndArray()
        .EndDict()
        .Build()
    },
    std::cout
  );
    std::cout << std::endl;

    json::Print(
        json::Document{
            json::Builder{}
            .Value("just a string")
            .Build()
        },
        std::cout
    );
    std::cout << std::endl;
  try {
    const Document input = json::Load(std::cin);
    const Dict& root = GetRootDict(input);
    TransportCatalogue catalogue;
    const Array& base_requests = GetBaseRequests(root);
    ParseStops(base_requests, catalogue);
    ParseDistances(base_requests, catalogue);
    ParseBuses(base_requests, catalogue);
    const renderer::RenderSettings settings =
      renderer::ParseRenderSettings(GetRenderSettingsNode(root));
    renderer::MapRenderer renderer(settings);
    svg::Document doc = renderer.Render(catalogue);
    std::ostringstream my_out;
    doc.Render(my_out);
    const json::Document output = json_reader::ProcessRequests(input, my_out.str() );
    json::Print(output, std::cout);
    return 0;
  }
  catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}