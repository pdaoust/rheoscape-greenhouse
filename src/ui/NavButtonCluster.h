#ifndef RHEOSCAPE_NAV_BUTTON_CLUSTER_H
#define RHEOSCAPE_NAV_BUTTON_CLUSTER_H

#include <event_stream/EventStream.h>
#include <event_stream/FancyPushbutton.h>

enum NavButton {
  navbutton_up,
  navbutton_down,
  navbutton_left,
  navbutton_right,
  navbutton_ok,
  navbutton_back
};

struct NavButtonClusterEvent {
  NavButton button;
  FancyPushbuttonEvent event;

  NavButtonClusterEvent(NavButton button, FancyPushbuttonEvent event)
  : button(button), event(event) { }

  bool isPressed(NavButton buttons) {
    return button | buttons && event == button_press;
  }
};

class NavButtonCluster : public EventStream<NavButtonClusterEvent> {
  public:
    NavButtonCluster(FancyPushbutton upButton, FancyPushbutton downButton, FancyPushbutton leftButton, FancyPushbutton rightButton, FancyPushbutton okButton, FancyPushbutton backButton) {
      upButton.registerSubscriber([this](Event<FancyPushbuttonEvent> event) { this->_emit(Event<NavButtonClusterEvent>(event.timestamp, NavButtonClusterEvent(navbutton_up, event.value))); });
      downButton.registerSubscriber([this](Event<FancyPushbuttonEvent> event) { this->_emit(Event<NavButtonClusterEvent>(event.timestamp, NavButtonClusterEvent(navbutton_down, event.value))); });
      leftButton.registerSubscriber([this](Event<FancyPushbuttonEvent> event) { this->_emit(Event<NavButtonClusterEvent>(event.timestamp, NavButtonClusterEvent(navbutton_left, event.value))); });
      rightButton.registerSubscriber([this](Event<FancyPushbuttonEvent> event) { this->_emit(Event<NavButtonClusterEvent>(event.timestamp, NavButtonClusterEvent(navbutton_right, event.value))); });
      okButton.registerSubscriber([this](Event<FancyPushbuttonEvent> event) { this->_emit(Event<NavButtonClusterEvent>(event.timestamp, NavButtonClusterEvent(navbutton_ok, event.value))); });
      backButton.registerSubscriber([this](Event<FancyPushbuttonEvent> event) { this->_emit(Event<NavButtonClusterEvent>(event.timestamp, NavButtonClusterEvent(navbutton_back, event.value))); });
    }
};

#endif