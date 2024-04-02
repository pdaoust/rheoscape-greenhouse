#ifndef RHEOSCAPE_MENU_H
#define RHEOSCAPE_MENU_H

#include <event_stream/EventStream.h>
#include <ui/NavButtonCluster.h>
#include <ui/Widget.h>

template <typename TBitmap>
struct MenuItem {
  Widget<TBitmap> widget;
  StateInput<bool> selected;
};

template <typename TBitmap>
class Menu : public InteractiveWidget { 
  private:
    std::vector<MenuItem<TBitmap>> _items;
    uint8_t _selectedItemIndex;

    virtual std::optional<TBitmap> _draw(StyleRule style) {

    };

    virtual bool _handleSelectedEventAndShouldBubble(Event<NavButtonClusterEvent> event) {
      switch (true) {
        case event.event.isPressed(navbutton_up):
          _selectNextItem(-1);
          break;

        case event.event.isPressed(navbutton_down):
          _selectNextItem(1);
          break;
      }

      // Never bubble, because all the children's event streams are already piped through this one.
      // Including the selected child.
      // FIXME: Is this a performance problem? Should I be doing some sort of smart routing of events instead?
      return false;
    }
  
    void _selectNextItem(int8_t direction) {
      uint8_t previouslySelectedItem = _selectedIndex;
      uint8_t i = _selectedItemIndex;
      while (i > 0 && i < _items.size() - 1) {
        i += direction;
        if (_items[i].isSelectable()) {
          _selectedItemIndex = i;
          _items[i].selected.write(true);
          _items[previouslySelectedItem].selected.write(false);
          return;
        }
      }
    }

  public:
    Menu(Input<bool> enabled, Input<bool> visible, Input<bool> selected, Input<WidgetStyleRules> styleRules, EventStream<NavButtonClusterEvent> eventStream)
    : InteractiveWidget(enabled, visible, selected, styleRules, eventStream)
    { }

    void addChild(std::function<(Input<bool>), Widget<TBitmap>> bind) {
      StateInput<bool> selected();
      FunctionInput<bool> conditionalSelected([this, selected]() { return this.isSelected() && selected.read(); });
      MenuItem<TBitmap> menuItem = {
        bind(conditionalSelected),
        selected;
      };
      _items.push_back(menuItem);
    }
};

#endif