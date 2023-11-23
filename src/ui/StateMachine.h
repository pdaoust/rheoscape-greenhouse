#ifndef RHEOSCAPE_STATE_MACHINE_H
#define RHEOSCAPE_STATE_MACHINE_H

#include <optional>
#include <vector>
#include <map>

#include <event_stream/EventStream.h>

template <typename TEvent, typename TState>
struct State {
  const std::map<TEvent, TState> transitionTable;
  const std::optional<std::function<void()>> onEnter;
  const std::optional<std::function<void()>> onExit;
};

template <typename TEvent, typename TState>
class StateMachine {
  private:
    std::map<TState, State<TEvent, TState>> _stateTable;
    std::optional<TState> _currentState;

  protected:
    void _handleEvent(Event<TEvent> event) {
      TState newState = _stateTable[_currentState][event];
      _transitionToState(newState);
    }

    void _transitionToState(TState newState) {
      if (std::has_value(_currentState)) {
        _stateTable[std::get(_currentState)].onExit();
      }
      _currentState = newState;
      _stateTable[_currentState].onEnter();
    }

  public:
    StateMachine(EventStream<TEvent> eventStream, std::optional<TState> startingState, std::map<TState, State<TEvent, TState>> stateTable)
    :
      _stateTable(stateTable),
      _currentState(std::nullopt)
    {
      eventStream.registerSubscriber([this](Event<TEvent> event) { this->_handleEvent(event); });
      _transitionToState(startingState);
    }

    StateMachine(EventStream<TEvent> eventStream, std::map<TState, State<TEvent, TState>> stateTable)
    : StateMachine(eventStream, std::nullopt, stateTable)
    { }
};

#endif