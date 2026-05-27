#pragma once 
//2026-04-26 21:58
#include <string> 
#include <vector> 
#include <algorithm>

#include "svg.h" 
#include "json.h" 
#include "transport_catalogue.h" 

namespace renderer {

  struct RenderSettings {
    double width = 600.0;
    double height = 400.0;
    double padding = 50.0;
    double line_width = 14.0;
    double stop_radius = 5.0;
    int bus_label_font_size = 20;
    svg::Point bus_label_offset{ 7.0, 15.0 };
    int stop_label_font_size = 20;
    svg::Point stop_label_offset{ 7.0, -3.0 };
    svg::Color underlayer_color = std::string("white");
    double underlayer_width = 3.0;
    std::vector<svg::Color> color_palette;
  };

  constexpr double EPSILON = 1e-6;

  inline bool IsZero(double value) {
    return std::abs(value) < EPSILON;
  }

  class SphereProjector {
    double padding_{ 0.0 };
    double min_lon_{ 0.0 };
    double max_lat_{ 0.0 };
    double zoom_coeff_{ 0.0 };

  public:
    template <typename PointInputIt>
    SphereProjector(PointInputIt points_begin, PointInputIt points_end,
      double max_width, double max_height, double padding)
      : padding_(padding) {
      if (points_begin == points_end) {
        return;
      }
      const auto [left_it, right_it] = std::minmax_element(
        points_begin, points_end,
        [](const auto& lhs, const auto& rhs) {
          return lhs.lng < rhs.lng;
        });
      min_lon_ = left_it->lng;
      const double max_lon = right_it->lng;
      const auto [bottom_it, top_it] = std::minmax_element(
        points_begin, points_end,
        [](const auto& lhs, const auto& rhs) {
          return lhs.lat < rhs.lat;
        });
      const double min_lat = bottom_it->lat;
      max_lat_ = top_it->lat;
      std::optional<double> width_zoom;
      if (!IsZero(max_lon - min_lon_)) {
        width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
      }
      std::optional<double> height_zoom;
      if (!IsZero(max_lat_ - min_lat)) {
        height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
      }
      if (width_zoom && height_zoom) {
        zoom_coeff_ = std::min(*width_zoom, *height_zoom);
      }
      else if (width_zoom) {
        zoom_coeff_ = *width_zoom;
      }
      else if (height_zoom) {
        zoom_coeff_ = *height_zoom;
      }
    }

    svg::Point operator()(geo::Coordinates coords) const {
      return {
          (coords.lng - min_lon_) * zoom_coeff_ + padding_,
          (max_lat_ - coords.lat) * zoom_coeff_ + padding_
      };
    }
  };

  RenderSettings ParseRenderSettings(const json::Dict& dict);

  class MapRenderer {
    RenderSettings settings_;

  public:
    explicit MapRenderer(RenderSettings settings);
    svg::Document Render(const transport_catalogue::TransportCatalogue& catalogue) const;
    void UnderlayLabelSets(
      const svg::Point pos, const std::string name, 
      const svg::Point offset, const int font_size, const svg::Color color, 
      svg::Document& doc, const std::string bold = "") const;
    void CreateBusPolyline(
      const transport_catalogue::TransportCatalogue& catalogue,
      SphereProjector& projector, svg::Document& doc) const;
    void CreateBusNumbersOnRoute(
      const transport_catalogue::TransportCatalogue& catalogue,
      SphereProjector& projector, svg::Document& doc) const;
    void CreateStopsOnRoute(
      const transport_catalogue::TransportCatalogue& catalogue,
      SphereProjector& projector, svg::Document& doc) const;
    void CreateStopsNameOnRoute(
      const transport_catalogue::TransportCatalogue& catalogue,
      SphereProjector& projector, svg::Document& doc) const;
  };
}  // namespace renderer 