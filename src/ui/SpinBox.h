#ifndef RHEOSCAPE_SPIN_BOX_H
#define RHEOSCAPE_SPIN_BOX_H

#include <Range.h>
#include <event_stream/EventStream.h>
#include <event_stream/FancyPushbutton.h>
#include <input/Input.h>
#include <ui/NavButtonCluster.h>
#include <ui/Widget.h>

template <typename TNum, typename TBitmap>
class SpinBox : public InteractiveWidget<TBitmap, NavButtonEvent> {
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

    virtual bool _handleEventAndShouldBubble(Event<NavButtonEvent> event) {
      switch (true) {
        case _focused.read():
          switch (true) {
            // Increment on all press events -- short, long, and hold-repeat.
            case (event.value.button & navbutton_up | navbutton_right) > 0 && event.value.event == button_press:
              _changeBy(1);
              // This widget captures the up/down events, naturally.
              return false;
            
            case (event.value.button & navbutton_down | navbutton_left) > 0 && event.value.event == button_press:
              _changeBy(-1);
              return false;
            
            case event.value.button == navbutton_ok && event.value.event == button_press:
              _value.write(_editBuffer.value());
              _focused.write(false);
              return false;
            
            case event.value.button == navbutton_back && event.value.event == button_press:
              _focused.write(false);
              return false;
            
            default:
              // I think I've covered all buttons, but just in case I expand the nav button cluster enum later...
              // Unhandled buttons prob shouldn't be captured.
              return true;
          }
        
        // When an item is selected but not focused, capture the ok event only to start editing.
        case selected.read() && event.value.button == navbutton_ok && event.value.event == button_press:
          _editBuffer = _value.read();
          _focused.write(true);
      }
    }
  
  public:
    SpinBox(BasicStateInput<TNum> value, BasicInput<TNum> step, BasicInput<Range<TNum>> range)
    :
      _value(value),
      _step(step),
      _range(range)
    { }
};

#endif