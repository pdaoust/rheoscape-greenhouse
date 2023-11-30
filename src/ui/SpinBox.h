#ifndef RHEOSCAPE_SPIN_BOX_H
#define RHEOSCAPE_SPIN_BOX_H

#include <Range.h>
#include <event_stream/EventStream.h>
#include <event_stream/FancyPushbutton.h>
#include <input/Input.h>
#include <ui/NavButtonCluster.h>
#include <ui/Widget.h>

template <typename TNum, typename TBitmap>
class SpinBox : public EditableWidget<TBitmap, NavButtonClusterEvent> {
  protected:
    BasicStateInput<TNum> _value;
    BasicInput<TNum> _step;
    std::optional<TNum> _editBuffer;
    BasicInput<Range<TNum>> _range;

    virtual std::optional<TBitmap> _draw() {

    }

    void _changeBy(int16_t steps) {
      Range<TNum> range = _range.read();
      TNum nextValue = _editBuffer.value() + _step.value() * steps;
      if (nextValue >= range.min && nextValue <= range.max) {
        _editBuffer.value = nextValue;
      }
    }

    virtual bool _handleEditEventAndShouldBubble(Event<NavButtonClusterEvent> event) {
      switch (true) {
        // Increment on all press events -- short, long, and hold-repeat.
        case event.value.isPressed(navbutton_up | navbutton_right):
          _changeBy(1);
          // This widget captures the up/down events, naturally.
          return false;
        
        case event.value.isPressed(navbutton_down | navbutton_left):
          _changeBy(-1);
          return false;
        
        default:
          return true;
      }
    }
  
  public:
    SpinBox(BasicStateInput<TNum> value, BasicInput<TNum> step, BasicInput<Range<TNum>> range, BasicInput<bool> enabled, BasicInput<bool> visible, BasicInput<bool> selected, BasicStateInput<bool> editing, BasicInput<WidgetStyleRules> styleRules, EventStream<TEvent> eventStream)
    :
      EditableWidget(value, enabled, visible, selected, editing, styleRules, eventStream),
      _step(step),
      _range(range)
    { }
};

#endif