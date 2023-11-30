#ifndef RHEOSCAPE_MENU_H
#define RHEOSCAPE_MENU_H

#include <event_stream/EventStream.h>
#include <ui/NavButtonCluster.h>
#include <ui/Widget.h>

template <typename TBitmap>
class Menu : public InteractiveWidget { 
  private:
    std::vector<Widget> _items;
    std::vector<uint8_t> _selectableItems;
    uint8_t _selectedItemIndex;

    virtual std::optional<TBitmap> _draw(StyleRule style) {

    };

    virtual bool _handleEventAndShouldBubble(Event<NavButtonClusterEvent> event) {
      switch (true) {
        case event.event.isPressed(navbutton_up):
          if (_selectedItemIndex == 0) {
            // Already at the top of the list.
            return false;
          }

      }
    }

  public:
    Menu(std::vector<Widget> items, BasicInput<bool> enabled, BasicInput<bool> visible, BasicInput<bool> selected, BasicInput<WidgetStyleRules> styleRules, EventStream<NavButtonClusterEvent> eventStream)
    : InteractiveWidget(enabled, visible, selected, styleRules, eventStream), _items(items) {
      // Enumerate the selectable menu items' vector indices into a list.
      // This saves us a lot of time later.
      for (uint8_t i = 0; i < items.size(); i ++) {
        if (items[i].isSelectable())
          _selectableItems[] = i;
      }      
    }
};

#endif