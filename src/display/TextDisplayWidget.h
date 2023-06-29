#ifndef RHEOSCAPE_TEXT_DISPLAY_WIDGET_H
#define RHEOSCAPE_TEXT_DISPLAY_WIDGET_H

#include <display/Canvas.h>
#include <input/Input.h>
#include <Process.h>

class TextDisplayWidget : public Input<ColouredBitmap1> {
  private:
    Input<std::tuple<std::string, Colour>> _input;
    Input<Colour> _colourInput;
    const BoxedText _textTemplate;

  public:
    TextDisplayWidget(
      Input<std::tuple<std::string, Colour>> input,
      const Coords size,
      const uint8_t fontSize,
      const Alignment alignment = left
    ) :
      _input(input),
      _textTemplate(std::string(""), Colour(), fontSize, size, alignment)
    { }

    TextDisplayWidget(
      Input<std::string> textInput,
      Input<Colour> colourInput,
      const Coords size,
      const uint8_t fontSize,
      const Alignment alignment = left
    )
    : TextDisplayWidget(
      Merging2NotEmptyProcess<std::string, Colour>(textInput, colourInput),
      size,
      fontSize,
      alignment
    )
    { }

    std::optional<ColouredBitmap1> read() {
      auto textAndColour = _input.read();
      if (!textAndColour.has_value()) {
        return std::nullopt;
      };

      BoxedText full = _textTemplate.copyWith(std::get<0>(textAndColour.value()));
      Canvas1 canvas(full.getBoxDimensions());
      canvas.drawText(Coords{0, 0}, full);
      return ColouredBitmap1::fromBitmap1(canvas.getBitmap(), FgBgColour(std::get<1>(textAndColour.value())));
    }
};

class Label : public TextDisplayWidget {
  public:
    Label(
      std::string text,
      uint8_t fontSize,
      Colour colour,
      std::optional<uint8_t> length = std::nullopt, 
      Alignment alignment = left
    )
    :
      TextDisplayWidget(
        ConstantInput<std::string>(text),
        ConstantInput<Colour>(colour),
        Coords{
          // If not given a space to fill, default to the size of the text.
          length.value_or(text.size()),
          // Labels are always one row tall. Sorry, but that's just the way it is.
          1
        },
        fontSize,
        alignment
      )
    { }
};

template <typename T>
std::string formatNumber(T value, uint8_t base, uint8_t precision) {
  std::stringstream text;
  text << std::setbase(base) << std::setprecision(precision) << value << std::endl;
  return text.str();
};

template <typename T>
class NumberToStringProcess : public TranslatingNotEmptyProcess<T, std::string> {
  public:
    NumberToStringProcess(Input<T> wrappedInput, uint8_t base, uint8_t precision)
    :
      TranslatingNotEmptyProcess<T, std::string>(
        wrappedInput,
        [base, precision](T value) {
          return formatNumber<T>(value.value(), base, precision);
        }
      )
    { }
};

template <typename T>
class NumberToStringProcessWithColours : public TranslatingNotEmptyProcess<T, std::tuple<std::string, Colour>> {
  private:
    std::map<T, Colour> _initializerListToBreakpointsMap(std::initializer_list<std::tuple<T, Colour>> breakpoints) {
      std::map<T, Colour> breakpointsMap;
      for (auto i : breakpoints) {
        breakpointsMap[std::get<0>(i)] = std::get<1>(i);
      }
      return breakpointsMap;
    }

  public:
    NumberToStringProcessWithColours(
      Input<T> wrappedInput,
      uint8_t base,
      uint8_t precision,
      Colour startColour,
      std::map<T, Colour> breakpoints
    ) : TranslatingNotEmptyProcess<T, std::tuple<std::string, Colour>>(
      wrappedInput,
      [base, precision, startColour, breakpoints](T value) {
        Colour colour = startColour;
        for (auto const& [v, c] : breakpoints) {
          if (value >= v) {
            colour = c;
          }
        }

        return std::tuple<std::string, Colour>{
          formatNumber(value, base, precision),
          colour        
        };
      }
    )
    { }

    NumberToStringProcessWithColours(
      Input<T> wrappedInput,
      uint8_t base,
      uint8_t precision,
      Colour startColour,
      std::initializer_list<std::tuple<T, Colour>> breakpoints
    ) : NumberToStringProcessWithColours(wrappedInput, base, precision, startColour, _initializerListToBreakpointsMap(breakpoints))
    { }
};

#endif