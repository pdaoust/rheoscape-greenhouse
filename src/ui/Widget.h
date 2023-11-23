#ifndef RHEOSCAPE_WIDGET_H
#define RHEOSCAPE_WIDGET_H

#include <input/Input.h>
#include <ui/ColourScheme.h>

// A base class for all widgets.
// As it is, it's only really useful for titles and help text, and that's it.
template <typename TBitmap>
class Widget : public Input<TBitmap> {
  private:
    BasicInput<WidgetStyleRules> _styleRules;

  protected:
    BasicInput<bool> _enabled;
    BasicInput<bool> _visible;

    virtual StyleRule _chooseStyleRule() {
      WidgetStyleRules styleRules = _styleRules.read();
      if (!_enabled.read()) {
        return styleRules.disabled;
      } else {
        return styleRules.base;
      }
    }

    virtual std::optional<TBitmap> _draw(StyleRule style);

  public:
    Widget(BasicInput<bool> enabled, BasicInput<bool> visible, BasicInput<WidgetStyleRules> styleRules)
    : _selected(selected), _enabled(enabled), _visible(visible), _styleRules(styleRules) { }

    std::optional<TBitmap> read() {
      if (!visible.read()) {
        return std::nullopt;
      }

      return _draw(_chooseStyleRule());
    }
};

// A non-interactive widget that nevertheless can have a selected style --
// for example, the label of an input widget in a menu item.
template <typename TBitmap>
class SelectableWidget : public Widget<TBitmap> {
  private:
    BasicInput<SelectableWidgetStyleRules> _styleRules;

  protected:
    BasicInput<bool> _selected;

    virtual std::optional<StyleRule> _chooseStyleRule() {
      SelectableWidgetStyleRules styleRules = _styleRules.read();
      WidgetStyleRules styleRules = _styleRules.read().value();
      if (!_enabled.read()) {
        return styleRules.disabled;
      } else if (_selected.read()) {
        return styleRules.selected;
      } else {
        return styleRules.base;
      }
    }

  public:
    SelectableWidget(BasicInput<bool> enabled, BasicInput<bool> visible, BasicInput<bool> selected, BasicInput<SelectableWidgetStyleRules> styleRules)
    : Widget(enabled, visible, styleRules), _selected(selected) { }
};

// An input of some sort that can have focus.
template <typename TBitmap, typename TEvent>
class InteractiveWidget : public SelectableWidget<TBitmap>, public EventStream<TEvent> {
  private:
    BasicInput<InteractiveWidgetStyleRules> _styleRules;

  protected:
    BasicStateInput<bool> _focused;

    virtual StyleRule _chooseStyleRule() {
      InteractiveWidgetStyleRules styleRules = _styleRules.read();
      if (!_enabled.read()) {
        return styleRules.disabled;
      } else if (_focused.read()) {
        return styleRules.focused;
      } else if (_selected.read()) {
        return styleRules.selected;
      } else {
        return styleRules.base;
      }
    }

    // Handle event and decide whether to bubble it.
    // The return value says whether to bubble.
    // Probably a code smell, but I don't care.
    virtual bool _handleEventAndShouldBubble(Event<TEvent> event) { }

  public:
    InteractiveWidget(BasicInput<bool> enabled, BasicInput<bool> visible, BasicInput<bool> selected, BasicStateInput<bool> focused, BasicInput<WidgetStyleRules> styleRules, EventStream<TEvent> eventStream)
    : Widget(enabled, visible, selected, styleRules), _focused(focused) {
      eventStream.registerSubscriber([this](Event<TEvent> event) {
        if (this->_handleEventAndShouldBubble(event)) {
          this->_emit(event);
        }
      });
    }
};

#endif