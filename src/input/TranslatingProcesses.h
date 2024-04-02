#ifndef RHEOSCAPE_TRANSLATING_PROCESSES_H
#define RHEOSCAPE_TRANSLATING_PROCESSES_H

#include <input/Input.h>

template <typename TIn, typename TOut>
class TranslatingProcess : public BasicInput<TOut> {
  private:
    BasicInput<TIn> _wrappedInput;
    const std::function<TOut(TIn)>& _translator;

  public:
    TranslatingProcess(BasicInput<TIn> wrappedInput, const std::function<TOut(TIn)>& translator)
    :
      _wrappedInput(wrappedInput),
      _translator(translator)
    { }

    TOut read() {
      return _translator(_wrappedInput.read());
    }
};

template <typename TIn, typename TOut>
class TranslatingNotEmptyProcess : public TranslatingProcess<std::optional<TIn>, TOut> {
  public:
    TranslatingNotEmptyProcess(BasicInput<std::optional<TIn>> wrappedInput, const std::function<TOut(std::optional<TIn>)>& translator)
    : TranslatingProcess<TIn, TOut>(wrappedInput, [translator](std::optional<TIn> value) { return value.has_value() ? translator(value.value()) : value; })
    { }
};

#endif