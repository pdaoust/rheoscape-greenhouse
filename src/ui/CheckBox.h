#ifndef RHEOSCAPE_CHECK_BOX_H
#define RHEOSCAPE_CHECK_BOX_H

#include <ui/Widget.h>

template <typename TBitmap>
class CheckBox : public SelectableWidget<TBitmap, NavButtonClusterEvent> {
  private:
    StateInput<bool> _value;

    virtual std::optional<TBitmap> _draw() {

    }

    void _toggle() {
      _value.write(!_value.read());
    }

    virtual bool _handleEventAndShouldBubble(Event<NavButtonClusterEvent> event) {
      switch (true) {
        case event.value.isPressed(button_ok):
          _toggle();
          return false;
        default:
          return true;
      }
    }
  
  public:
    CheckBox(StateInput<bool> value, Input<bool> enabled, Input<bool> visible, Input<bool> selected, Input<WidgetStyleRules> styleRules, EventStream<TEvent> eventStream)
    : InteractiveWidget(enabled, visible, selected, styleRules, eventStream), _value(value) { }
};

#endif