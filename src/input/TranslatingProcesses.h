#ifndef RHEOSCAPE_TRANSLATING_PROCESSES_H
#define RHEOSCAPE_TRANSLATING_PROCESSES_H

#include <input/Input.h>

template <typename TIn, typename TOut>
class TranslatingProcess : public Input<TOut> {
  private:
    Input<TIn> _wrappedInput;
    const std::function<std::optional<TOut>(std::optional<TIn>)>& _translator;

  public:
    TranslatingProcess(Input<TIn> wrappedInput, const std::function<std::optional<TOut>(std::optional<TIn>)>& translator)
    :
      _wrappedInput(wrappedInput),
      _translator(translator)
    { }

    std::optional<TOut> read() {
      return _translator(_wrappedInput.read());
    }
};

template <typename TIn, typename TOut>
class TranslatingNotEmptyProcess : public TranslatingProcess<TIn, TOut> {
  public:
    TranslatingNotEmptyProcess(Input<TIn> wrappedInput, const std::function<TOut(TIn)>& translator)
    : TranslatingProcess<TIn, TOut>(wrappedInput, [translator](std::optional<TIn> value) { return value.has_value() ? translator(value.value()) : value; })
    { }
};

#endif