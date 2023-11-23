#ifndef RHEOSCAPE_CHECK_BOX_H
#define RHEOSCAPE_CHECK_BOX_H

#include <ui/Widget.h>

template <typename TBitmap>
class CheckBox : public InteractiveWidget<TBitmap, NavButtonEvent> {
  private:
    BasicInput<bool> _value;

    virtual std::optional<TBitmap> _draw() {
      
    }
};

#endif