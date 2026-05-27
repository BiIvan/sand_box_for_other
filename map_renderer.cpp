//2026-04-26 21:58
#include <cmath> 
#include <string> 
#include <vector> 
#include <utility> 
#include <optional>  
#include <stdexcept> 

#include "map_renderer.h" 

namespace renderer {

    svg::Color ParseColor(const json::Node& node) {
      if (node.IsString()) {
        return node.AsString();
      }
      if (node.IsArray()) {
        const auto& arr = node.AsArray();
        if (arr.size() == 3) {
          return svg::Rgb(
            static_cast<uint8_t>(arr[0].AsInt()),
            static_cast<uint8_t>(arr[1].AsInt()),
            static_cast<uint8_t>(arr[2].AsInt())
          );
        }
        if (arr.size() == 4) {
          return svg::Rgba(
            static_cast<uint8_t>(arr[0].AsInt()),
            static_cast<uint8_t>(arr[1].AsInt()),
            static_cast<uint8_t>(arr[2].AsInt()),
            arr[3].AsDouble()
          );
        }
      }
      throw std::runtime_error("Invalid color format in render_settings");
    }

    svg::Point ParseOffset(const json::Node& node) {
      const auto& arr = node.AsArray();
      if (arr.size() != 2) {
        throw std::runtime_error("Offset must contain exactly two elements");
      }
      return { arr[0].AsDouble(), arr[1].AsDouble() };
    }

    std::vector<geo::Coordinates> CollectAllRouteCoordinates(
      const transport_catalogue::TransportCatalogue& catalogue) {
      std::vector<geo::Coordinates> result;
      for (const auto& bus : catalogue.GetAllBuses()) {
        for (const Stop* stop : bus.stops_) {
          if (stop) {
            result.push_back(stop->coordinates);
          }
        }
      }
      return result;
    }

    std::vector<const Stop*> CollectUniqueStops(
      const transport_catalogue::TransportCatalogue& catalogue) {
      std::vector<const Stop*> stops;
      for (const auto& bus : catalogue.GetAllBuses()) {
        for (const Stop* stop : bus.stops_) {
          if (stop) {
            stops.push_back(stop);
          }
        }
      }
      std::sort(stops.begin(), stops.end());
      stops.erase(std::unique(stops.begin(), stops.end()), stops.end());
      std::sort(stops.begin(), stops.end(), [](const Stop* lhs, const Stop* rhs) {
        return lhs->name_ < rhs->name_;
        });
      return stops;
    }

  RenderSettings ParseRenderSettings(const json::Dict& dict) {
    RenderSettings settings;
    if (dict.count("width")) {
      settings.width = dict.at("width").AsDouble();
    }
    if (dict.count("height")) {
      settings.height = dict.at("height").AsDouble();
    }
    if (dict.count("padding")) {
      settings.padding = dict.at("padding").AsDouble();
    }
    if (dict.count("line_width")) {
      settings.line_width = dict.at("line_width").AsDouble();
    }
    if (dict.count("stop_radius")) {
      settings.stop_radius = dict.at("stop_radius").AsDouble();
    }
    if (dict.count("bus_label_font_size")) {
      settings.bus_label_font_size = dict.at("bus_label_font_size").AsInt();
    }
    if (dict.count("bus_label_offset")) {
      settings.bus_label_offset = ParseOffset(dict.at("bus_label_offset"));
    }
    if (dict.count("stop_label_font_size")) {
      settings.stop_label_font_size = dict.at("stop_label_font_size").AsInt();
    }
    if (dict.count("stop_label_offset")) {
      settings.stop_label_offset = ParseOffset(dict.at("stop_label_offset"));
    }
    if (dict.count("underlayer_color")) {
      settings.underlayer_color = ParseColor(dict.at("underlayer_color"));
    }
    if (dict.count("underlayer_width")) {
      settings.underlayer_width = dict.at("underlayer_width").AsDouble();
    }
    if (dict.count("color_palette")) {
      for (const auto& color_node : dict.at("color_palette").AsArray()) {
        settings.color_palette.push_back(ParseColor(color_node));
      }
    }
    if (settings.color_palette.empty()) {
      settings.color_palette.push_back(std::string("black"));
    }
    return settings;
  }

  MapRenderer::MapRenderer(RenderSettings settings)
    : settings_(std::move(settings)) {
    double pad = std::min(settings_.width, settings_.height) / 2;

    settings_.width < 0 || settings_.width > 100000 
      ? settings_.width = 640.0 
      : settings_.width;

    settings_.height < 0 || settings_.height > 100000 
      ? settings_.height = 480.0 
      : settings_.height;

    settings_.padding < 0 || settings_.padding > pad 
      ? settings_.padding = 60.0 
      : settings_.padding;

    settings_.line_width < 0 || settings_.line_width > 100000 
      ? settings_.line_width = 13.0 
      : settings_.line_width;

    settings_.stop_radius < 0 || settings_.stop_radius > 100000 
      ? settings_.stop_radius = 4.0 
      : settings_.stop_radius;

    settings_.bus_label_font_size < 0 || settings_.bus_label_font_size > 100000 
      ? settings_.bus_label_font_size = 20 
      : settings_.bus_label_font_size;

    (settings_.bus_label_offset.x < -100000) || (settings_.bus_label_offset.x > 100000) 
      ? settings_.bus_label_offset.x = 0.0 
      : settings_.bus_label_offset.x;

    (settings_.bus_label_offset.y < -100000) || (settings_.bus_label_offset.y > 100000) 
      ? settings_.bus_label_offset.y = 0.0 
      : settings_.bus_label_offset.y;

    settings_.stop_label_font_size < 0 || settings_.stop_label_font_size > 100000 
      ? settings_.stop_label_font_size = 20 
      : settings_.stop_label_font_size;

    (settings_.stop_label_offset.x < -100000) || (settings_.stop_label_offset.x > 100000) 
      ? settings_.stop_label_offset.x = 0.0 
      : settings_.stop_label_offset.x;

    (settings_.stop_label_offset.y < -100000) || (settings_.stop_label_offset.y > 100000) 
      ? settings_.stop_label_offset.y = 0.0 
      : settings_.stop_label_offset.y;

    settings_.underlayer_width < 0 || settings_.underlayer_width > 100000 
      ? settings_.underlayer_width = 3.0 
      : settings_.underlayer_width;
  }

  void MapRenderer::UnderlayLabelSets(
    const svg::Point pos,
    const std::string name,
    const svg::Point offset,
    const int font_size,
    const svg::Color color,
    svg::Document& doc,
    const std::string bold) const {

    svg::Text underlayer;
    underlayer.SetPosition(pos);
    underlayer.SetOffset(offset);
    underlayer.SetFontSize(font_size);
    underlayer.SetFontFamily("Verdana");
    underlayer.SetFontWeight(bold);
    underlayer.SetData(name);
    underlayer.SetFillColor(settings_.underlayer_color);
    underlayer.SetStrokeColor(settings_.underlayer_color);
    underlayer.SetStrokeWidth(settings_.underlayer_width);
    underlayer.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
    underlayer.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
    doc.Add(underlayer);
    svg::Text label;
    label.SetPosition(pos);
    label.SetOffset(offset);
    label.SetFontSize(font_size);
    label.SetFontFamily("Verdana");
    label.SetFontWeight(bold);
    label.SetData(name);
    label.SetFillColor(color);
    doc.Add(label);
  }

  void MapRenderer::CreateBusPolyline(
    const transport_catalogue::TransportCatalogue& catalogue, 
    SphereProjector& projector, svg::Document& doc ) const{

    size_t color_index = 0;
    for (const auto& bus : catalogue.GetAllBuses()) {
      if (bus.stops_.empty()) {
        continue;
      }
      svg::Polyline polyline;
      for (const Stop* stop : bus.stops_) {
        polyline.AddPoint(projector(stop->coordinates));
      }
      if (!bus.is_roundtrip_ && bus.stops_.size() > 1) {
        for (auto it = std::next(bus.stops_.rbegin()); it != bus.stops_.rend(); ++it) {
          polyline.AddPoint(projector((*it)->coordinates));
        }
      }
      polyline.SetFillColor(svg::NoneColor);
      polyline.SetStrokeColor(
        settings_.color_palette[color_index % settings_.color_palette.size()]
      );
      polyline.SetStrokeWidth(settings_.line_width);
      polyline.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
      polyline.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
      doc.Add(polyline);
      ++color_index;
    }
  }

  void MapRenderer::CreateBusNumbersOnRoute(
    const transport_catalogue::TransportCatalogue& catalogue,
    SphereProjector& projector, svg::Document& doc) const {

    size_t color_index = 0;
    for (const auto& bus : catalogue.GetAllBuses()) {
      if (bus.stops_.empty()) {
        continue;
      }
      const svg::Color bus_color =
        settings_.color_palette[color_index % settings_.color_palette.size()];
      auto render_label = [&](const Stop* stop) {
        UnderlayLabelSets(
          projector(stop->coordinates), 
          std::string(bus.name_), 
          settings_.bus_label_offset, 
          settings_.bus_label_font_size, 
          bus_color, doc, "bold");
        };
      render_label(bus.stops_.front());
      if (!bus.is_roundtrip_ && bus.stops_.front() != bus.stops_.back()) {
        render_label(bus.stops_.back());
      }
      ++color_index;
    }
  }

  void MapRenderer::CreateStopsOnRoute(
    const transport_catalogue::TransportCatalogue& catalogue,
    SphereProjector& projector, svg::Document& doc) const {

    const auto stops = CollectUniqueStops(catalogue);
    for (const Stop* stop : stops) {
      svg::Circle circle;
      circle.SetCenter(projector(stop->coordinates));
      circle.SetRadius(settings_.stop_radius);
      circle.SetFillColor(std::string("white"));
      doc.Add(circle);
    }
  }

  void MapRenderer::CreateStopsNameOnRoute(
    const transport_catalogue::TransportCatalogue& catalogue,
    SphereProjector& projector, svg::Document& doc) const {

    const auto stops = CollectUniqueStops(catalogue);
    for (const Stop* stop : stops) {
      UnderlayLabelSets(
        projector(stop->coordinates), 
        std::string(stop->name_), 
        settings_.stop_label_offset, 
        settings_.stop_label_font_size, 
        std::string("black"), doc);
    }
  }

  svg::Document MapRenderer::Render(
    const transport_catalogue::TransportCatalogue& catalogue) const {

    svg::Document doc;
    const auto points = CollectAllRouteCoordinates(catalogue);
    SphereProjector projector(
      points.begin(),
      points.end(),
      settings_.width,
      settings_.height,
      settings_.padding
    );
    CreateBusPolyline(catalogue, projector, doc);
    CreateBusNumbersOnRoute(catalogue, projector, doc);
    CreateStopsOnRoute(catalogue, projector, doc);
    CreateStopsNameOnRoute(catalogue, projector, doc);
    return doc;
  }

}  // namespace renderer 
