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

struct NavButtonEvent {
  NavButton button;
  FancyPushbuttonEvent event;

  NavButtonEvent(NavButton button, FancyPushbuttonEvent event)
  : button(button), event(event) { }
};

class NavButtonCluster : public EventStream<NavButtonEvent> {
  public:
    NavButtonCluster(FancyPushbutton upButton, FancyPushbutton downButton, FancyPushbutton leftButton, FancyPushbutton rightButton, FancyPushbutton okButton, FancyPushbutton backButton) {
      upButton.registerSubscriber([this](Event<FancyPushbuttonEvent> event) { this->_emit(Event<NavButtonEvent>(event.timestamp, NavButtonEvent(navbutton_up, event.value))); });
      downButton.registerSubscriber([this](Event<FancyPushbuttonEvent> event) { this->_emit(Event<NavButtonEvent>(event.timestamp, NavButtonEvent(navbutton_down, event.value))); });
      leftButton.registerSubscriber([this](Event<FancyPushbuttonEvent> event) { this->_emit(Event<NavButtonEvent>(event.timestamp, NavButtonEvent(navbutton_left, event.value))); });
      rightButton.registerSubscriber([this](Event<FancyPushbuttonEvent> event) { this->_emit(Event<NavButtonEvent>(event.timestamp, NavButtonEvent(navbutton_right, event.value))); });
      okButton.registerSubscriber([this](Event<FancyPushbuttonEvent> event) { this->_emit(Event<NavButtonEvent>(event.timestamp, NavButtonEvent(navbutton_ok, event.value))); });
      backButton.registerSubscriber([this](Event<FancyPushbuttonEvent> event) { this->_emit(Event<NavButtonEvent>(event.timestamp, NavButtonEvent(navbutton_back, event.value))); });
    }
};

#endif