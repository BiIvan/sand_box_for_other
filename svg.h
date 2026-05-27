#pragma once
//2026-04-26 10:34

#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include <variant>
#include <iostream>
#include <optional>
#include <string_view>

namespace svg {

  namespace detail {

    template <typename T>
    inline void RenderValue(std::ostream& out, const T& value) {
      out << value;
    }

    void HtmlEncodeString(std::ostream& out, std::string_view sv);

    template <>
    inline void RenderValue<std::string>(std::ostream& out, const std::string& s) {
      HtmlEncodeString(out, s);
    }

    template <typename AttrType>
    inline void RenderAttr(std::ostream& out, std::string_view name, const AttrType& value) {
      using namespace std::literals;
      out << name << "=\""sv;
      RenderValue(out, value);
      out.put('"');
    }

    template <typename AttrType>
    inline void RenderOptionalAttr(std::ostream& out, std::string_view name,
      const std::optional<AttrType>& value) {
      if (value) {
        RenderAttr(out, name, *value);
      }
    }

  }  // namespace detail

  struct Point {
    Point() = default;
    Point(double x, double y)
      : x(x)
      , y(y) {
    }
    const std::string ToString() const {
      return std::to_string(x) + "," + std::to_string(y);
    }
    double x = 0.0;
    double y = 0.0;
  };

  inline std::ostream& operator<<(std::ostream& tuo, const Point& p) {
    return tuo << p.x << ',' << p.y;
  }

  struct Rgb {
    Rgb() = default;
    Rgb(uint8_t r, uint8_t g, uint8_t b)
      : red(r)
      , green(g)
      , blue(b) {
    }
    const std::string ToString() const {
      return '"' + "rgb("
        + std::to_string(red) + ","
        + std::to_string(green) + ","
        + std::to_string(blue) + ","
        + '"';
    }
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
  };

  inline std::ostream& operator<<(std::ostream& tuo, const Rgb& rgb) {
    return tuo << '"' << "rgb(" << rgb.red << ',' << rgb.green << ',' << rgb.blue << ")" << '"';
  }

  struct Rgba {
    Rgba() = default;
    Rgba(uint8_t r, uint8_t g, uint8_t b, double a)
      : red(r)
      , green(g)
      , blue(b)
      , opacity(a) {
    }
    const std::string ToString() const {
      return '"' + "rgba("
        + std::to_string(red) + ","
        + std::to_string(green) + ","
        + std::to_string(blue) + ","
        + std::to_string(opacity) + ")"
        + '"';
    }
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
    double opacity = 1.0;
  };

  inline std::ostream& operator<<(std::ostream& tuo, const Rgba& rgb) {
    return tuo << '"' << "rgba(" << rgb.red << ',' << rgb.green << ',' << rgb.blue << ',' << rgb.opacity << ")" << '"';
  }

  using Color = std::variant<std::monostate, std::string, Rgb, Rgba>;
  inline const Color NoneColor{};

  std::ostream& operator<<(std::ostream& out, const Color& color);

  struct RenderContext {
    RenderContext(std::ostream& out)
      : out(out) {
    }

    RenderContext(std::ostream& out, int indent_step, int indent = 0)
      : out(out)
      , indent_step(indent_step)
      , indent(indent) {
    }

    RenderContext Indented() const {
      return { out, indent_step, indent + indent_step };
    }

    void RenderIndent() const {
      for (int i = 0; i < indent; ++i) {
        out.put(' ');
      }
    }

    std::ostream& out;
    int indent_step = 0;
    int indent = 0;
  };

  class Object {
    virtual void RenderObject(const RenderContext& context) const = 0;

  public:
    void Render(const RenderContext& context) const;
    virtual ~Object() = default;
  };

  enum class StrokeLineCap {
    BUTT,
    ROUND,
    SQUARE,
  };

  std::ostream& operator<<(std::ostream& out, StrokeLineCap value);

  enum class StrokeLineJoin {
    ARCS,
    BEVEL,
    MITER,
    MITER_CLIP,
    ROUND,
  };

  std::ostream& operator<<(std::ostream& out, StrokeLineJoin value);

  template <typename Owner>
  class PathProps {
    Owner& AsOwner() {
      return static_cast<Owner&>(*this);
    }
    std::optional<Color> fill_color_;
    std::optional<Color> stroke_color_;
    std::optional<double> stroke_width_;
    std::optional<StrokeLineCap> stroke_line_cap_;
    std::optional<StrokeLineJoin> stroke_line_join_;

  public:
    Owner& SetFillColor(Color color) {
      fill_color_ = std::move(color);
      return AsOwner();
    }
    Owner& SetStrokeColor(Color color) {
      stroke_color_ = std::move(color);
      return AsOwner();
    }
    Owner& SetStrokeWidth(double width) {
      stroke_width_ = width;
      return AsOwner();
    }
    Owner& SetStrokeLineCap(StrokeLineCap line_cap) {
      stroke_line_cap_ = line_cap;
      return AsOwner();
    }
    Owner& SetStrokeLineJoin(StrokeLineJoin line_join) {
      stroke_line_join_ = line_join;
      return AsOwner();
    }

  protected:
    ~PathProps() = default;
    void RenderAttrs(std::ostream& out) const {
      using detail::RenderOptionalAttr;
      using namespace std::literals;
      RenderOptionalAttr(out, "fill"sv, fill_color_);
      RenderOptionalAttr(out, " stroke"sv, stroke_color_);
      RenderOptionalAttr(out, " stroke-width"sv, stroke_width_);
      RenderOptionalAttr(out, " stroke-linecap"sv, stroke_line_cap_);
      RenderOptionalAttr(out, " stroke-linejoin"sv, stroke_line_join_);
    }
  };

  class Circle : public Object, public PathProps<Circle> {
    Point center_;
    double radius_ = 1.0;  
    void RenderObject(const RenderContext& context) const override;

  public:
    Circle& SetCenter(Point center);
    Circle& SetRadius(double radius);
  };

  class Polyline : public Object, public PathProps<Polyline> {
    std::vector<Point> points_;
    void RenderObject(const RenderContext& context) const override;

  public:
    Polyline& AddPoint(Point point);
  };

  class Text : public Object, public PathProps<Text> {
    Point position_;
    Point offset_;
    uint32_t font_size_ = 1;
    std::string font_family_;
    std::string font_weight_;
    std::string data_;
    void RenderObject(const RenderContext& context) const override;

  public:
    Text& SetPosition(Point pos);
    Text& SetOffset(Point offset);
    Text& SetFontSize(uint32_t size);
    Text& SetFontFamily(std::string font_family);
    Text& SetFontWeight(std::string font_weight);
    Text& SetData(std::string data);
  };

  class ObjectContainer {
  public:
    template <typename ObjectType>
    void Add(ObjectType object) {
      AddPtr(std::make_unique<ObjectType>(std::move(object)));
    }
    virtual void AddPtr(std::unique_ptr<Object>&& obj) = 0;

  protected:
    ~ObjectContainer() = default;
  };

  class Drawable {
  public:
    virtual void Draw(ObjectContainer& container) const = 0;
    virtual ~Drawable() = default;
  };

  class Document : public ObjectContainer {
    std::vector<std::unique_ptr<Object>> objects_;

  public:
    void AddPtr(std::unique_ptr<Object>&& obj) override;
    void Render(std::ostream& out) const;
  };

}  // namespace svg