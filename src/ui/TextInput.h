#ifndef RHEOSCAPE_TEXT_INPUT_H
#define RHEOSCAPE_TEXT_INPUT_H

#include <string>

#include <display/Bitmap.h>
#include <event_stream/EventStream.h>
#include <input/Input.h>
#include <ui/NavButtonCluster.h>
#include <ui/StateMachine.h>
#include <ui/Widget.h>

template <typename TBitmap>
class TextInputWidget : public InteractiveWidget<TBitmap, NavButtonEvent> {
  private:
    Input<std::string> _value;
    uint8_t _cursorPosition;
    std::optional<std::string> _editBuffer;
    StateMachine<NavButtonEvent, TextInputWidgetState> _stateMachine;

  protected: 
    std::optional<TBitmap> _draw() {

    }

    void _moveCursor(int8_t amount) {
      if (amount < 0) {
        if (_cursorPosition + amount < 0) {
          _cursorPosition = 0;
        } else {
          _cursorPosition += amount;
        }
      } else if (amount > 0) {
        // If unwrapping causes an exception, it really should be an exceptional case.
        uint8_t bufferLength = _editBuffer.value().length();
        if (_cursorPosition + amount > bufferLength) {
          _cursorPosition = bufferLength;
        } else {
          _cursorPosition += amount;
        }
      }
    }

    void _insertText(std::string text) {

    }

    void _startEditing() {
      _focused.write(true);
      _editBuffer = _value.read().value_or("");
    }

    void _stopEditing() {
      _focused.write(false);
      _editBuffer = std::nullopt;
    }

    void _save() {
      // Again, it should be an exceptional case that _editBuffer is empty in this state.
      _value.write(_editBuffer.value());
    }

  public:
    TextInputWidget(StateInput<std::string> value, Input<bool> enabled, Input<bool> visible, Input<bool> selected, StateInput<bool> focused, Input<WidgetStyleRules> styleRules, EventStream<TEvent> eventStream)
    : InteractiveWidget(enabled, visible, selected, styleRules, eventStream), _value(value) { }
};

#endif