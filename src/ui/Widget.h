#ifndef RHEOSCAPE_WIDGET_H
#define RHEOSCAPE_WIDGET_H

#include <input/Input.h>
#include <ui/ColourScheme.h>

// A base class for all widgets.
// As it is, it's only really useful for titles and help text, and that's it.
template <typename TBitmap>
class Widget : public Input<TBitmap> {
  private:
    Input<WidgetStyleRules> _styleRules;

  protected:
    Input<bool> _enabled;
    Input<bool> _visible;
    Input<bool> _selected;

    virtual StyleRule _chooseStyleRule() {
      WidgetStyleRules styleRules = _styleRules.read();
      if (_selected.read()) {
        return styleRules.selected;
      } else if (_enabled.read()) {
        return styleRules.base;
      } else {
        return styleRules.disabled;
      }
    }

    virtual std::optional<TBitmap> _draw(StyleRule style);

  public:
    Widget(Input<bool> enabled, Input<bool> visible, Input<WidgetStyleRules> styleRules)
    : _selected(selected), _enabled(enabled), _visible(visible), _styleRules(styleRules) { }

    Widget(Input<WidgetStyleRules> styleRules)
    : Widget(Input<bool>(), Input<bool>(), styleRules) { }

    void rebind(Input<bool> enabled, Input<bool> visible) {
      _enabled = enabled;
      _visible = visible;
    }

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
};

template <typename TBitmap>
class InteractiveWidget : public Widget<TBitmap>, public EventStream<NavButtonClusterEvent> {
  protected:
    virtual bool _handleEventAndShouldBubble(Event<NavButtonClusterEvent> event) {
      if (_selected.read()) {
        return _handleSelectedEventAndShouldBubble(event);
      }
      return true;
    }

    // Override this in your class.
    virtual bool _handleSelectedEventAndShouldBubble(Event<NavButtonClusterEvent> event) {
      return true;
    }

  public:
    InteractiveWidget(Input<bool> enabled, Input<bool> visible, Input<bool> selected, Input<WidgetStyleRules> styleRules, EventStream<NavButtonClusterEvent> eventStream)
    : Widget(enabled, visible, selected, styleRules) {
      eventStream.registerSubscriber([this](Event<NavButtonClusterEvent> event) {
        if (this->_handleEventAndShouldBubble(event)) {
          this->_emit(event);
        }
      });
    }

    InteractiveWidget(Input<SelectableWidgetStyleRules> styleRules, EventStream<NavButtonClusterEvent> eventStream)
    : InteractiveWidget(Input<bool>(), Input<bool>(), Input<bool>(), styleRules, eventStream) { }

    virtual bool isSelectable() {
      return isEnabled();
    }
};

// Any sort of interactive widget that requres a press of the 'OK' button
// to enter its
template <typename TBitmap>
class EnterableWidget : public InteractiveWidget<TBitmap> {
  protected:
    bool _entered;

    virtual bool _handleSelectedEventAndShouldBubble(Event<NavButtonClusterEvent> event) {
      switch (true) {
        case event.event.isPressed(navbutton_ok):
          // We always capture the 'OK' button because it lets us enter and exit the widget.
          if (!_entered) {
            _entered = true;
            _handleEnter();
          } else {
            _entered = false;
            _handleExit(true);
          }
          return false;
        
        case event.event.isPressed(navbutton_back):
          if (!_entered) {
            return true;
          } else {
            _entered = false;
            _handleExit(false);
            return false;
          }

        default:
          if (_entered) {
            return _handleEnteredEventAndShouldBubble(event);
          }
          return true;
      }
    }

    // Override this function to handle all button events that your widget should capture.
    virtual bool _handleEnteredEventAndShouldBubble(Event<NavButtonClusterEvent> event) {
      return true;
    }

    // Override this function if you need to do any setup.
    virtual bool _handleEnter() { }

    // Override this function if you need to do any teardown.
    virtual void _handleExit(bool okPressed) { }

  public:
    EnterableWidget(Input<bool> enabled, Input<bool> visible, Input<bool> selected, Input<WidgetStyleRules> styleRules, EventStream<NavButtonClusterEvent> eventStream)
    : InteractiveWidget(enabled, visible, selected, styleRules, eventStream) { }
};

// An input of some sort that, when you enter it, allows a value to be edited.
// Make sure you override _handleEnteredEventAndShouldBubble() in derived classes.
template <typename TBitmap, typename TEditable>
class EditableWidget : public EnterableWidget<TBitmap> {
  private:
    Input<WidgetStyleRules> _styleRules;
    StateInput<TEditable> _value;
    std::optional<TEditable> _editBuffer;

  protected:
    virtual void handleEnter() {
      _editBuffer = _value.read();
    }

    virtual void handleExit(bool okPressed) {
      if (okPressed) {
        _value.write(_editBuffer.value());
      } else {
        _editBuffer = std::nullopt;
      }
    }

  public:
    EditableWidget(StateInput<TValue> value, Input<bool> enabled, Input<bool> visible, Input<bool> selected, Input<WidgetStyleRules> styleRules, EventStream<NavButtonClusterEvent> eventStream)
    : EnterableWidget(enabled, visible, selected, styleRules), _value(value) { }
};

#endif