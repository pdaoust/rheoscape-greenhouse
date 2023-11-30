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

    bool isEnabled() {
      return _enabled.read();
    }

    virtual bool isSelectable() {
      return false;
    }

    virtual bool isEnterable() {
      return false;
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

    bool isSelected() {
      return _selected.read();
    }

    virtual bool isSelectable() {
      return _enabled.read();
    }
};

template <typename TBitmap>
class InteractiveWidget : public Widget<TBitmap>, public EventStream<NavButtonClusterEvent> {
  protected:
    virtual bool _handleEventAndShouldBubble(Event<NavButtonClusterEvent> event) {
      return true;
    }

  public:
    InteractiveWidget(BasicInput<bool> enabled, BasicInput<bool> visible, BasicInput<bool> selected, BasicInput<WidgetStyleRules> styleRules, EventStream<NavButtonClusterEvent> eventStream)
    : Widget(enabled, visible, selected, styleRules) {
      eventStream.registerSubscriber([this](Event<NavButtonClusterEvent> event) {
        if (this->_handleEventAndShouldBubble(event)) {
          this->_emit(event);
        }
      });
    }
};

template <typename TBitmap>
class EnterableWidget : public InteractiveWidget<TBitmap> {
  protected:
    BasicStateInput<bool> _entered;

    virtual bool _handleEventAndShouldBubble(Event<NavButtonClusterEvent> event) {
      if (!_enabled.read()) {
        return true;
      }

      switch (true) {
        case event.event.isPressed(navbutton_ok):
          _entered.write(true);
          return false;
      }
    }

  public:
    EnterableWidget()

    bool isEnterable() {
      return enabled.read();
    }
};

// An input of some sort that can have focus.
template <typename TBitmap, typename TEditable>
class EditableWidget : public InteractiveWidget<TBitmap> {
  private:
    BasicInput<EditableWidgetStyleRules> _styleRules;
    BasicStateInput<TEditable> _value;
    std::optional<TEditable> _editBuffer;

  protected:
    virtual StyleRule _chooseStyleRule() {
      EditableWidgetStyleRules styleRules = _styleRules.read();
      if (!_enabled.read()) {
        return styleRules.disabled;
      } else if (_editing.read()) {
        return styleRules.editable;
      } else if (_selected.read()) {
        return styleRules.selected;
      } else {
        return styleRules.base;
      }
    }

    // Handle event and decide whether to bubble it.
    // The return value says whether to bubble.
    // Probably a code smell, but I don't care.
    virtual bool _handleEventAndShouldBubble(Event<NavButtonClusterEvent> event) {
      switch (true) {
        case _editing.read():
          switch (true) {
            case event.value.isPressed(navbutton_ok):
              _value.write(_editBuffer.value());
              _editing.write(false);
              return false;
            
            case event.value.isPressed(navbutton_back):
              _editBuffer = std::nullopt;
              _editing.write(false);
              return false;
            
            default:
              return _handleEditEventAndShouldBubble(event);
          }
        
        // When an item is selected but not focused, capture the ok event only to start editing.
        case selected.read() && event.value.isPressed(navbutton_ok):
          _editBuffer = _value.read();
          _editing.write(true);
      }
    }

    virtual bool _handleEditEventAndShouldBubble(Event<NavButtonClusterEvent> event) {
      return true;
    }

  public:
    EditableWidget(BasicStateInput<TValue> value, BasicInput<bool> enabled, BasicInput<bool> visible, BasicInput<bool> selected, BasicStateInput<bool> editing, BasicInput<WidgetStyleRules> styleRules, EventStream<NavButtonClusterEvent> eventStream)
    : InteractiveWidget(enabled, visible, selected, styleRules), _value(value), _editing(editing) { }
};

#endif