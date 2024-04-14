#ifndef RHEOSCAPE_TRANSLATING_PROCESSES_H
#define RHEOSCAPE_TRANSLATING_PROCESSES_H

#include <functional>

#include <input/Input.h>

template <typename TIn, typename TOut>
class TranslatingProcess : public Input<TOut> {
  private:
    Input<TIn>* _wrappedInput;
    std::function<TOut(TIn)> _translator;

  public:
    TranslatingProcess(Input<TIn>* wrappedInput, std::function<TOut(TIn)> translator)
    :
      _wrappedInput(wrappedInput),
      _translator(translator)
    { }

    virtual TOut read() {
      return _translator(_wrappedInput->read());
    }
};

template <typename TIn, typename TOut>
class TranslatingOptionalProcess : public TranslatingProcess<std::optional<TIn>, std::optional<TOut>> {
  public:
    TranslatingOptionalProcess(
      Input<std::optional<TIn>>* wrappedInput,
      std::function<TOut(TIn)> translator
    )
    : TranslatingProcess<std::optional<TIn>, std::optional<TOut>>(
        wrappedInput,
        [translator](std::optional<TIn> value) {
          return value.has_value()
            ? (std::optional<TOut>)translator(value.value())
            : std::nullopt;
        })
    { }
};

#endif