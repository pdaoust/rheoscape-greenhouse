#ifndef RHEOSCAPE_RADIO_BUTTON_H
#define RHEOSCAPE_RADIO_BUTTON_H

#include <ui/Widget.h>

template <typename TBitmap, typename TOption>
class RadioButton : public SelectableWidget<TBitmap, NavButtonClusterEvent> {
  private:
    StateInput<TOption> _selectedOption;
    TOption _optionId;

    virtual std::optional<TBitmap> _draw() {

    }

    void _toggle() {
      _selectedOption.write(optionId);
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
    RadioButton(StateInput<TOption> selectedOption, TOption optionId, Input<bool> enabled, Input<bool> visible, Input<bool> selected, Input<WidgetStyleRules> styleRules, EventStream<TEvent> eventStream)
    : InteractiveWidget(enabled, visible, selected, styleRules, eventStream), _selectedOption(selectedOption), _optionId(optionId) { }
};

#endif