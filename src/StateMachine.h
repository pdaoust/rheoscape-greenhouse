#ifndef RHEOSCAPE_STATE_MACHINE_H
#define RHEOSCAPE_STATE_MACHINE_H

#include <map>
#include <optional>
#include <input/Input.h>
#include <event_stream/EventStream.h>

template <typename TEvent, typename TState>
class StateMachine {
  private:
    StateInput<TState> _stateContainer;
    std::map<TEvent, std::function<void(std::optional<TState>)>&> _transitionFunctions;

  public:
    StateMachine(
      EventStream<TEvent> eventStream,
      StateInput<TState> stateContainer,
      std::map<TEvent, std::function<void(std::optional<TState>)>&> transitionFunctions
    ) :
      _stateContainer(stateContainer),
      _transitionFunctions(transitionFunctions)
    {
      eventStream.subscribe(receiveEvent);
    }

    void receiveEvent(Event<TEvent> event) {
      if (_transitionFunctions.find(event.value) != _transitionFunctions.end()) {
        std::function<void(std::optional<TState>)>& transitionFunction = _transitionFunctions.at(event.value);
        transitionFunction(_stateContainer);
      }
    }
};

#endif