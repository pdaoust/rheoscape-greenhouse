#ifndef RHEOSCAPE_THEME_H
#define RHEOSCAPE_THEME_H

#include <display/DrawingPrimitives.h>

class StyleRule {
  private:
    std::optional<StyleRule&> _parent;
    std::optional<Colour> _fgColour;
    std::optional<Colour> _bgColour;
    std::optional<Colour> _lineColour;
    std::optional<uint8_t> _lineWidth;
    std::optional<uint8_t> _cornerRadius;
    std::optional<uint8_t> _padding;
    std::optional<uint8_t> _fontSize;
  
  public:
    StyleRule(
      std::optional<Colour> fgColour = std::nullopt,
      std::optional<Colour> bgColour = std::nullopt,
      std::optional<Colour> lineColour = std::nullopt,
      std::optional<uint8_t> lineWidth = std::nullopt,
      std::optional<uint8_t> cornerRadius = std::nullopt,
      std::optional<uint8_t> padding = std::nullopt,
      std::optional<uint8_t> fontSize = std::nullopt,
      std::optional<StyleRule&> parent = std::nullopt
    ) :
      _fgColour(fgColour),
      _bgColour(bgColour),
      _lineColour(lineColour),
      _lineWidth(lineWidth),
      _cornerRadius(cornerRadius),
      _padding(padding),
      _fontSize(fontSize),
      _parent(parent)
    { }

    StyleRule(StyleRule &rule)
    : StyleRule(rule._fgColour, rule._bgColour, rule._lineColour, rule._lineWidth, rule._cornerRadius, rule._padding, rule._fontSize, rule._parent) { }

    // This bit of gnarliness is necessary because Colour has some const properties and can't be copied.
    // And, in the words of someone else, the semantics of std::optional is broken in that regard.
    void fgColour(std::optional<Colour> fgColour) {
      if (fgColour.has_value()) {
        _fgColour.emplace(fgColour.value());
      } else {
        _fgColour = std::nullopt;
      }
    }
    const std::optional<Colour> fgColour() {
      return _fgColour.has_value() ? _fgColour.value() :
        _parent.has_value() ? _parent.value().fgColour() : std::nullopt;
    }

    void bgColour(std::optional<Colour> bgColour) {
      if (bgColour.has_value()) {
        _bgColour.emplace(bgColour.value());
      } else {
        _bgColour = std::nullopt;
      }
    }
    const std::optional<Colour> bgColour() {
      return _bgColour.has_value() ? _bgColour.value() :
        _parent.has_value() ? _parent.value().bgColour() : std::nullopt;
    }

    void lineColour(std::optional<Colour> lineColour) {
      if (lineColour.has_value()) {
        _lineColour.emplace(lineColour.value());
      } else {
        _lineColour = std::nullopt;
      }
    }
    const std::optional<Colour> lineColour() {
      return _lineColour.has_value() ? _lineColour.value() :
        _parent.has_value() ? _parent.value().lineColour() : std::nullopt;
    }

    void lineWidth(std::optional<uint8_t> lineWidth) {
      if (lineWidth.has_value()) {
        _lineWidth.emplace(lineWidth.value());
      } else {
        _lineWidth = std::nullopt;
      }
    }
    const std::optional<uint8_t> lineWidth() {
      return _lineWidth.has_value() ? _lineWidth.value() :
        _parent.has_value() ? _parent.value().lineWidth() : std::nullopt;
    }

    void cornerRadius(std::optional<uint8_t> cornerRadius) {
      if (cornerRadius.has_value()) {
        _cornerRadius.emplace(cornerRadius.value());
      } else {
        _cornerRadius = std::nullopt;
      }
    }
    const std::optional<uint8_t> cornerRadius() {
      return _cornerRadius.has_value() ? _cornerRadius.value() :
        _parent.has_value() ? _parent.value().cornerRadius() : std::nullopt;
    }

    void padding(std::optional<uint8_t> padding) {
      if (padding.has_value()) {
        _padding.emplace(padding.value());
      } else {
        _padding = std::nullopt;
      }
    }
    const std::optional<uint8_t> padding() {
      return _padding.has_value() ? _padding.value() :
        _parent.has_value() ? _parent.value().padding() : std::nullopt;
    }

    void fontSize(std::optional<uint8_t> fontSize) {
      if (fontSize.has_value()) {
        _fontSize.emplace(fontSize.value());
      } else {
        _fontSize = std::nullopt;
      }
    }
    const std::optional<uint8_t> fontSize() {
      return _fontSize.has_value() ? _fontSize.value() :
        _parent.has_value() ? _parent.value().fontSize() : std::nullopt;
    }
};

struct WidgetStyleRules {
  StyleRule base;
  StyleRule disabled;

  WidgetStyleRules(StyleRule base, StyleRule disabled)
  : base(base), disabled(disabled) { }
};

struct InteractiveWidgetStyleRules : public WidgetStyleRules {
  StyleRule selected;

  InteractiveWidgetStyleRules(StyleRule base, StyleRule disabled, StyleRule selected)
  : WidgetStyleRules(base, disabled), selected(selected) { }
};

struct EditableWidgetStyleRules : public SelectableWidgetStyleRules {
  StyleRule editing;

  EditableWidgetStyleRules(StyleRule base, StyleRule disabled, StyleRule selected, StyleRule editing)
  : InteractiveWidgetStyleRules(base, disabled, selected), editing(editing) { }
};

struct Theme {
  WidgetStyleRules header;
  // Non-interactive text -- input labels, for instance.
  WidgetStyleRules text;
  // Text/number boxes and unopened select boxes.
  EditableWidgetStyleRules input;
  // Wrapper around things that appear in a menu, like labels and inputs.
  InteractiveWidgetStyleRules menuItem;
  InteractiveWidgetStyleRules tab;
  InteractiveWidgetStyleRules button;
  // Checkbox or radio button.
  InteractiveWidgetStyleRules checkAndRadio;
  WidgetStyleRules modalContainer;
};

#endif