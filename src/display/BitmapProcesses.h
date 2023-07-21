#ifndef RHEOSCAPE_BITMAP_PROCESSES_H
#define RHEOSCAPE_BITMAP_PROCESSES_H

#include <display/Bitmap.h>
#include <display/Canvas.h>
#include <input/Input.h>

template <typename TBitmapIn>
class BitmapBoxingProcess : public Input<Bitmap16> {
  private:
    Input<std::tuple<TBitmapIn, FgBgColour>> _wrappedInput;
    uint8_t _cornerRadius;
    Margins _padding;

  public:
    BitmapBoxingProcess(Input<std::tuple<TBitmapIn, FgBgColour>> wrappedInput, uint8_t cornerRadius, Margins padding)
    :
      _wrappedInput(wrappedInput),
      _cornerRadius(cornerRadius),
      _padding(padding)
    { }

    BitmapBoxingProcess(Input<TBitmapIn> bitmapInput, Input<FgBgColour> coloursInput, uint8_t cornerRadius, Margins padding)
    : BitmapBoxingProcess(Merging2NotEmptyProcess<TBitmapIn, FgBgColour>(bitmapInput, coloursInput), cornerRadius, padding)
    { }

    std::optional<Bitmap16> read() {
      std::optional<std::tuple<TBitmapIn, FgBgColour>> value = _wrappedInput.read();
      if (!value.has_value()) {
        return std::nullopt;
      }

      TBitmapIn bitmap = std::get<0>(value.value());
      FgBgColour colours = std::get<1>(value.value());

      Coords outerDimensions{
        bitmap.dimensions.x + _padding.left + _padding.right,
        bitmap.dimensions.y + _padding.top + _padding.bottom
      };

      Canvas16 image(outerDimensions);
      image.fillRoundRect(Coords{0, 0}, outerDimensions, _cornerRadius, colours.fgColour);
      if (colours.bgColour.has_value()) {
        image.drawRoundRect(Coords{0, 0}, outerDimensions, _cornerRadius, colours.bgColour.value());
      }
      image.drawBitmap<TBitmapIn>(bitmap);

      Canvas1 mask(outerDimensions);
      mask.fillRoundRect(Coords{0, 0}, outerDimensions, _cornerRadius, Colour(true));

      Bitmap16 imageBitmap = image.getBitmap();
      Bitmap1 maskBitmap = mask.getBitmap();
      return Bitmap16(
        imageBitmap.data,
        imageBitmap.dimensions,
        maskBitmap.data
      );
    }
};

template <typename TDisplayBitmap>
class BitmapPositioningProcess : public TranslatingNotEmptyProcess<std::tuple<TDisplayBitmap, Coords>, PositionedBitmap<TDisplayBitmap>> {
  public:
    BitmapPositioningProcess(Input<TDisplayBitmap> bitmapInput, Input<Coords> positionInput)
    :
      TranslatingNotEmptyProcess<std::tuple<TDisplayBitmap, Coords>, PositionedBitmap<TDisplayBitmap>>(
        Merging2NotEmptyProcess<TDisplayBitmap, Coords>(bitmapInput, positionInput),
        [](std::tuple<TDisplayBitmap, Coords> value) {
          return PositionedBitmap<TDisplayBitmap>{
            std::get<0>(value),
            std::get<1>(value)
          };
        }
      )
    { }

    BitmapPositioningProcess(Input<TDisplayBitmap> bitmapInput, Input<int16_t> xInput, Input<int16_t> yInput)
    :
      BitmapPositioningProcess(
        bitmapInput,
        TranslatingNotEmptyProcess(
          Merging2NotEmptyProcess<int16_t, int16_t>(xInput, yInput),
          [](std::tuple<int16_t, int16_t> value) {
            return Coords{std::get<0>(value), std::get<1>(value)};
          }
        )
      )
    { }
};

template <typename TDisplayBitmapIn, typename TSizedBitmapOut, typename TCanvas, uint16_t W, uint16_t H>
class CompositorProcess : public Input<TSizedBitmapOut> {
  private:
    std::vector<Input<PositionedBitmap<TDisplayBitmapIn>>> _inputs;

  public:
    CompositorProcess(std::vector<Input<PositionedBitmap<TDisplayBitmapIn>>> inputs)
    : _inputs(inputs)
    { }

    std::optional<TSizedBitmapOut> read() {
      TCanvas canvas(W, H);

      for (auto input : _inputs) {
        std::optional<PositionedBitmap<TDisplayBitmapIn>> value = input.read();
        if (value.has_value()) {
//          canvas.drawBitmap<TDisplayBitmapIn>(value.value().bitmap, value.value().topLeft);
        }
      }

      return TSizedBitmapOut(canvas.getBitmap());
    }
};

template <uint16_t W, uint16_t H>
using MonochromeCompositorProcess = CompositorProcess<DisplayBitmapMonochrome, SizedBitmap1<W, H>, Canvas1, W, H>;

template <uint16_t W, uint16_t H>
using ColourCompositorProcess = CompositorProcess<DisplayBitmapColour, SizedBitmap16<W, H>, Canvas16, W, H>;

using Ssd1306CompositorProcess_0_49_inches_landscape = MonochromeCompositorProcess<64, 32>;
using Ssd1306CompositorProcess_0_96_inches_landscape = MonochromeCompositorProcess<128, 64>;
using St7735CompositorProcess_0_96_inches_landscape = ColourCompositorProcess<160, 80>;
using St7735CompositorProcess_1_8_inches_portrait = ColourCompositorProcess<128, 160>;
using St7735CompositorProcess_1_8_inches_landscape = ColourCompositorProcess<160, 128>;
using Ili9341CompositorProcess_2_8_inches_portrait = ColourCompositorProcess<240, 320>;
using Ili9341CompositorProcess_2_8_inches_landscape = ColourCompositorProcess<320, 240>;

#endif