#ifndef PAULDAOUST_DISPLAYS_H
#define PAULDAOUST_DISPLAYS_H

#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <optional>
#include <vector>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_ILI9341.h>

#include <Input.h>
#include <Process.h>
#include <Runnable.h>

struct Coords {
  const int16_t x;
  const int16_t y;
};

struct Grid {
  const Coords origin;
  const int16_t xStep;
  const int16_t yStep;

  Coords getPixelCoordsAt(Coords gridCoords) const {
    return Coords {
      (int16_t)(origin.x + gridCoords.x * xStep),
      (int16_t)(origin.y + gridCoords.y * yStep)
    };
  }
};

enum BitDepth {
  one = 1,
  sixteen = 16
};

struct Colour {
  const uint8_t red;
  const uint8_t green;
  const uint8_t blue;

  Colour(uint8_t red, uint8_t green, uint8_t blue)
  :
    red(red),
    green(green),
    blue(blue)
  { }

  Colour(uint16_t colour)
  :
    red(colour & 0xF800 >> 11),
    green(colour & 0x7E0 >> 5),
    blue(colour & 0x1F)
  { }

  Colour(uint32_t colour) : Colour((uint8_t) colour >> 16, (uint8_t) colour & 0xFF00 >> 8, (uint8_t) colour & 0xFF) { }

  Colour(bool colour)
  : Colour(
    colour ? 0xFF : 0x00,
    colour ? 0xFF : 0x00,
    colour ? 0xFF : 0x00
  ) { }

  // The 'default' colour is black.
  Colour()
  : Colour(false)
  { }

  uint32_t to24Bit() const {
    return (uint32_t)(
      (red << 16)
      | (green << 8)
      | blue
    );
  }

  uint16_t to16Bit() const {
    return (uint16_t)(
      (red / 8 << 11)
      | (green / 4 << 5)
      | (blue / 8)
    );
  }

  bool to1Bit() const {
    return red > 127 || green > 127 || blue > 127;
  }
  // Reduce the colour to the appropriate bit depth, then expand to full-colour.
  // This makes it usable with monochrome displays (which treat anything > 0 as 1)
  // and colour displays.
  Colour toDisplayBitDepth(BitDepth bitDepth) const {
    switch (bitDepth) {
      case one:
        return Colour((uint32_t)(to1Bit() ? 0xFFFFFF : 0x000000));
      case sixteen:
        return this;
    }
  }
};

struct FgBgColour {
  const Colour fgColour;
  const std::optional<Colour> bgColour;
  
  FgBgColour(Colour fgColour, std::optional<Colour> bgColour = std::nullopt)
  :
    fgColour(fgColour),
    bgColour(bgColour)
  { }

  FgBgColour(uint32_t fgColour, std::optional<uint32_t> bgColour = std::nullopt)
  : FgBgColour(Colour(fgColour), bgColour.has_value() ? (std::optional<Colour>)Colour(bgColour.value()) : std::nullopt)
  { }

  FgBgColour()
  : FgBgColour(Colour())
  { }

  FgBgColour toDisplayBitDepth(BitDepth bitDepth) const {
    return FgBgColour(
      fgColour.toDisplayBitDepth(bitDepth),
      bgColour.has_value() ? bgColour.value().toDisplayBitDepth(bitDepth) : (std::optional<Colour>)std::nullopt
    );
  }
};

struct StyledText {
  const std::string text;
  const Colour colour;
  const uint8_t fontSize;

  StyledText(std::string text, Colour colour, uint8_t fontSize)
  :
    text(text),
    colour(colour),
    fontSize(fontSize)
  { }

  StyledText copyWith(std::optional<std::string> t, std::optional<Colour> c) const {
    return StyledText(
      t.has_value() ? t.value() : text,
      c.has_value() ? c.value() : colour,
      fontSize
    );
  }
};

enum Alignment {
  left,
  centre,
  right
};

struct Margins {
  const uint16_t top;
  const uint16_t right;
  const uint16_t bottom;
  const uint16_t left;

  Margins(uint16_t all)
  :
    top(all),
    right(all),
    bottom(all),
    left(all)
  { }

  Margins(uint16_t vertical, uint16_t horizontal)
  :
    top(vertical),
    right(horizontal),
    bottom(vertical),
    left(horizontal)
  { }

  Margins(uint16_t top, uint16_t horizontal, uint16_t bottom)
  :
    top(top),
    right(horizontal),
    bottom(bottom),
    left(horizontal)
  { }
};

struct BoxedText : StyledText {
  // This is the size of the bounding box in columns and rows, not pixels.
  const Coords size;
  const Alignment alignment;
  // What is shown when the text is bigger than the bounding box?
  const std::tuple<std::string, std::string> clipText;

  BoxedText(std::string text, Colour colour, uint8_t fontSize, Coords size, Alignment alignment = left, std::tuple<std::string, std::string> clipText = std::tuple<std::string, std::string>("<", ">"))
  :
    StyledText(text, colour, fontSize),
    size(size),
    alignment(alignment),
    clipText(clipText)
  { }    

  BoxedText(StyledText text, Coords size, Alignment alignment = left, std::tuple<std::string, std::string> clipText = std::tuple<std::string, std::string>("<", ">"))
  : BoxedText(text.text, text.colour, text.fontSize, size, alignment, clipText)
  { }

  BoxedText copyWith(std::optional<std::string> t, std::optional<Colour> c = std::nullopt) const {
    return BoxedText(
      t.has_value() ? t.value() : text,
      c.has_value() ? c.value() : colour,
      fontSize,
      size,
      alignment,
      clipText
    );
  }

  Coords getBoxDimensions() const {
    return Coords{
      (int16_t)(size.x * 6 * fontSize),
      (int16_t)(size.y * 8 * fontSize)
    };
  }

  uint16_t getCapacity() const {
    return size.x * size.y;
  }

  // TODO: make this respect word wrapping conventions.
  std::string formatTextWithClipAndAlignment() const {
    std::stringstream buf;
    if (text.length() > getCapacity()) {
      std::string clipped;
      switch (alignment) {
        case left:
          clipped = text.substr(0, getCapacity() - std::get<1>(clipText).length());
          buf << clipped << std::get<1>(clipText) << std::endl;
          return buf.str();
        case right:
          clipped = text.substr(0, getCapacity() - std::get<0>(clipText).length());
          buf << std::get<0>(clipText) << clipped << std::endl;
          return buf.str();
        case centre:
          clipped = text.substr(0, getCapacity() - std::get<0>(clipText).length() - std::get<1>(clipText).length());
          buf << std::get<0>(clipText) << clipped << std::get<1>(clipText) << std::endl;
          return buf.str();
      }
    } else {
      switch (alignment) {
        case left:
          buf << text << std::setw(getCapacity()) << std::endl;
          return buf.str();
        case right:
          buf << std::setw(getCapacity()) << text << std::endl;
          return buf.str();
        case centre:
          uint16_t padOneSide = getCapacity() - text.length() / 2;
          buf << std::setw(padOneSide + text.length()) << text << std::setw(getCapacity()) << std::endl;
          return buf.str();
      }
    }
  }
};

enum Rotation {
  deg_0 = 0,
  deg_90 = 1,
  deg_180 = 2,
  deg_270 = 3
};

bool rotationIsPerpendicular(Rotation rotation) {
  return (int)rotation % 2 == 1;
}

template <typename TData>
struct Bitmap {
  const TData* data;
  const Coords dimensions;
  const std::optional<uint8_t*> mask;

  Bitmap(const TData* data, const Coords dimensions, const std::optional<uint8_t*> mask = std::nullopt)
  :
    data(data),
    dimensions(dimensions),
    mask(mask)
  { }
};

template <uint16_t W, uint16_t H, typename TData, typename TBitmap>
struct SizedBitmap : TBitmap {
  SizedBitmap(const TData* data, const std::optional<uint8_t*> mask = std::nullopt)
  : TBitmap(data, Coords(W, H), mask)
  { }

  SizedBitmap(TBitmap unsized)
  : SizedBitmap(unsized.data, unsized.mask)
  { }
};

struct Bitmap1 : Bitmap<uint8_t> {
  Bitmap1(const uint8_t* data, const Coords dimensions, const std::optional<uint8_t*> mask = std::nullopt)
  : Bitmap<uint8_t>(data, dimensions, mask)
  { }

  ColouredBitmap1 withColours(FgBgColour colours) {
    return ColouredBitmap1(
      data,
      dimensions,
      colours,
      mask
    );
  }
};

template <uint16_t W, uint16_t H>
using SizedBitmap1 = SizedBitmap<W, H, uint8_t, Bitmap1>;

using Bitmap16 = Bitmap<uint16_t>;

template <uint16_t W, uint16_t H>
using SizedBitmap16 = SizedBitmap<W, H, uint16_t, Bitmap16>;

struct ColouredBitmap1 : Bitmap1 {
  const FgBgColour colours;

  ColouredBitmap1(const uint8_t* data, const Coords dimensions, const FgBgColour colours, const std::optional<uint8_t*> mask = std::nullopt)
  :
    Bitmap1(data, dimensions, mask),
    colours(colours)
  { }
};

template <uint16_t W, uint16_t H>
struct SizedColouredBitmap1 : SizedBitmap<W, H, uint8_t, ColouredBitmap1> {
  SizedColouredBitmap1(const uint8_t* data, const FgBgColour colours, const std::optional<uint8_t*> mask = std::nullopt)
  : ColouredBitmap1(data, Coords(W, H), colours, mask)
  { }

  SizedColouredBitmap1<W, H> withColours(FgBgColour colours) {
    return SizedColouredBitmap1<W, H>(
      data,
      colours,
      mask
    );
  }
};

// TODO: I don't like how this can allow full-colour styled bitmaps.
// Yes, RGB colours can be converted to the monochrome colourspace, but it feels ugly --
// especially when I have to specify a full RGB colour for white and black.
typedef std::variant<ColouredBitmap1, Bitmap1> DisplayBitmapMonochrome;

template <uint16_t W, uint16_t H>
using SizedDisplayBitmapMonochrome = std::variant<SizedColouredBitmap1<W, H>, SizedBitmap1<W, H>>;

// A bitmap for colour displays can be 1-bit (in which case 1 is mapped to white and 0 is mapped to black), styled 1-bit, or full-colour.
typedef std::variant<DisplayBitmapMonochrome, Bitmap16> DisplayBitmapColour;

template <uint16_t W, uint16_t H>
using SizedDisplayBitmapColour = std::variant<SizedDisplayBitmapMonochrome<W, H>, SizedBitmap16<W, H>>;

template <typename TBitmap>
struct PositionedBitmap {
  const TBitmap bitmap;
  const Coords topLeft;
};

typedef PositionedBitmap<DisplayBitmapMonochrome> PositionedBitmapMonochrome;

typedef PositionedBitmap<DisplayBitmapColour> PositionedBitmapColour;

class BitmapDrawingHelper {
  public:
    static void drawBitmapWithoutMask(Adafruit_GFX* driver, Coords topLeft, Bitmap1 bitmap) {
      driver->drawBitmap(
        topLeft.x,
        topLeft.y,
        bitmap.data,
        bitmap.dimensions.x,
        bitmap.dimensions.y,
        // Regardless of the bit depth of the driver, we use 16-bit white for the 'on' bits.
        // That's because the 1-bit drivers will still see it as white.
        // That makes this helper work for all display types.
        0xFFFF,
        0x0
      );
    }

    // Helper method to draw a styled bitmap, which is the same for all bit-depth displays.
    static void drawBitmapWithoutMask(Adafruit_GFX* driver, Coords topLeft, ColouredBitmap1 bitmap) {
      if (bitmap.colours.bgColour.has_value()) {
        driver->drawBitmap(
          topLeft.x,
          topLeft.y,
          bitmap.data,
          bitmap.dimensions.x,
          bitmap.dimensions.y,
          bitmap.colours.fgColour.to16Bit(),
          bitmap.colours.bgColour.value().to16Bit()
        );
      } else {
        // Transparent 'off' bits.
        driver->drawBitmap(
          topLeft.x,
          topLeft.y,
          bitmap.data,
          bitmap.dimensions.x,
          bitmap.dimensions.y,
          bitmap.colours.fgColour.to16Bit()
        );
      }
    }

    static void drawBitmapWithoutMask(Adafruit_GFX* driver, Coords topLeft, Bitmap16 bitmap) {
      driver->drawRGBBitmap(
        topLeft.x,
        topLeft.y,
        bitmap.data,
        bitmap.dimensions.x,
        bitmap.dimensions.y
      );
    }

    template <typename TBitmap>
    static void drawBitmapWithMask(Adafruit_GFX* driver, Coords topLeft, TBitmap bitmap) {
      // Regrdless of the bit depth of the underlying driver or the bitmap to be drawn,
      // draw the maskless bitmap onto a colour canvas.
      // This is because the only bitmap that can have a mask is a colour one.
      GFXcanvas16 colourCanvas(bitmap.dimensions.x, bitmap.dimensions.y);
      drawBitmapWithoutMask(colourCanvas, bitmap);

      // Once we've done that, draw it to the driver with the mask.
      // If the bitmap is monochrome and the driver is monochrome,
      // 0xFFFF will get turned into 1.
      driver.drawRGBBitmap(
        topLeft.x,
        topLeft.y,
        colourCanvas.getBuffer(),
        bitmap.mask.value(),
        bitmap.dimensions.x,
        bitmap.dimensions.y
      );
    }

    template <typename TBitmap>
    static void drawBitmap(Adafruit_GFX* driver, Coords topLeft, TBitmap bitmap) {
      if (bitmap.mask.has_value()) {
        drawBitmapWithMask(driver, bitmap);
      } else {
        drawBitmapWithoutMask(driver, bitmap);
      }
    }

    static void drawBitmap(Adafruit_GFX* driver, Coords topLeft, DisplayBitmapMonochrome bitmap) {
      if (std::holds_alternative<Bitmap1>(bitmap)) {
        drawBitmap(driver, topLeft, std::get<Bitmap1>(bitmap));
      } else {
        drawBitmap(driver, topLeft, std::get<ColouredBitmap1>(bitmap));
      }
    }

    static void drawBitmap(Adafruit_GFX* driver, Coords topLeft, DisplayBitmapColour bitmap) {
      if (std::holds_alternative<DisplayBitmapMonochrome>(bitmap)) {
        drawBitmap(driver, topLeft, std::get<DisplayBitmapMonochrome>(bitmap));
      } else {
        drawBitmap(driver, topLeft, std::get<Bitmap16>(bitmap));
      }
    }
};

template <typename TGfxCanvas, typename TBitmapOut, uint8_t TBitDepth>
class Canvas {
  private:
    TGfxCanvas _gfxCanvas;

  protected:
    Canvas(TGfxCanvas gfxCanvas) : _gfxCanvas(gfxCanvas) { }

    TGfxCanvas* _getGfxCanvas() {
      return &_gfxCanvas;
    }

  public:
    TBitmapOut getBitmap() {
      return TBitmapOut{
        _getGfxCanvas()->getBuffer(),
        getDimensions()
      };
    }

    Coords getDimensions() const {
      return Coords {
        _gfxCanvas.width(),
        _gfxCanvas.height()
      };
    }

    // My idiomatic versions of all the GFX primitives.
    void drawPixel(Coords coords, Colour colour) {
      _gfxCanvas.drawPixel(coords.x, coords.y, colour.toDisplayBitDepth(TBitDepth));
    }

    void setRotation(Rotation rotation) {
      _gfxCanvas.setRotation((int)rotation);
    }

    Rotation getRotation() {
      return (Rotation)_gfxCanvas.getRotation();
    }

    void invertDisplay(bool invert) {
      _gfxCanvas.invertDisplay(invert);
    }

    void drawFastVLine(Coords topLeft, int16_t height, Colour colour) {
      _gfxCanvas.drawFastVLine(origin.x, origin.y, height, colour.toDisplayBitDepth(TBitDepth));
    }

    void drawFastHLine(Coords origin, int16_t length, Colour colour) {
      _gfxCanvas.drawFastHLine(origin.x, origin.y, length, colour.toDisplayBitDepth(TBitDepth));
    }

    void fillRect(Coords corner1, Coords corner2, Colour colour) {
      _gfxCanvas.fillRect(corner1.x, corner1.y, corner2.x - corner1.x, corner2.y - corner1.y, colour.toDisplayBitDepth(TBitDepth));
    }

    void fillScreen(Colour colour) {
      _gfxCanvas.fillScreen(colour.toDisplayBitDepth(TBitDepth));
    }

    void drawLine(Coords start, Coords end, Colour colour) {
      _gfxCanvas.drawLine(start.x, start.y, end.x - start.x, end.y - start.y, colour.toDisplayBitDepth(TBitDepth));
    }

    void drawRect(Coords corner1, Coords corner2, Colour colour) {
      _gfxCanvas.drawRect(corner1.x, corner1.y, corner2.x - corner1.x, corner2.y - corner1.y, colour.toDisplayBitDepth(TBitDepth));
    }

    void drawCircle(Coords centre, int16_t radius, Colour colour) {
      _gfxCanvas.drawCircle(centre.x, centre.y, radius, colour.toDisplayBitDepth(TBitDepth));
    }

    void fillCircle(Coords centre, int16_t radius, Colour colour) {
      _gfxCanvas.fillCircle(centre.x, centre.y, radius, colour.toDisplayBitDepth(TBitDepth));
    }

    void drawTriangle(Coords point1, Coords point2, Coords point3, Colour colour) {
      _gfxCanvas.drawTriangle(point1.x, point1.y, point2.x, point2.y, point3.x, point3.y, colour.toDisplayBitDepth(TBitDepth));
    }

    void fillTriangle(Coords point1, Coords point2, Coords point3, Colour colour) {
      _gfxCanvas.fillTriangle(point1.x, point1.y, point2.x, point2.y, point3.x, point3.y, colour.toDisplayBitDepth(TBitDepth));
    }

    void drawRoundRect(Coords corner1, Coords corner2, int16_t radius, Colour colour) {
      _gfxCanvas.drawRoundRect(corner1.x, corner1.y, corner2.x - corner1.x, corner2.y - corner1.y, radius, colour.toDisplayBitDepth(TBitDepth));
    }

    void fillRoundRect(Coords corner1, Coords corner2, int16_t radius, Colour colour) {
      _gfxCanvas.fillRoundRect(corner1.x, corner1.y, corner2.x - corner1.x, corner2.y - corner1.y, radius, colour.toDisplayBitDepth(TBitDepth));
    }

    void drawText(Coords topLeft, BoxedText text) {
      // We use a 1-bit canvas for the bounding box, then position that canvas on the larger canvas.
      // We're doing this so we get clipping, wrapping, and eventually scrolling for free.
      GFXcanvas1 canvas(text.getBoxDimensions().x, text.getBoxDimensions().y);
      canvas.setCursor(0, 0);
      canvas.setTextColor(1, 0);
      // If the bounding box is only one row high, clip it.
      // If it's taller, turn wrapping on.
      canvas.setTextWrap(text.size.y > 1);

      // Fonty stuff.
      canvas.cp437(true);
      canvas.setTextSize(text.fontSize);

      // Now output the text!
      canvas.print(text.formatTextWithClipAndAlignment());

      _gfxCanvas.drawBitmap(
        topLeft.x,
        topLeft.y,
        canvas.getBuffer(),
        canvas.width(),
        canvas.height(),
        text.colour.toDisplayBitDepth(TBitDepth)
        // Never specify the background colour;
        // text is always transparent on its background.
        // The only value of specifying a background (that I can see)
        // is as a cheap crappy framebuffer for monospace text.
      );
    }

    template <typename TBitmapIn>
    void drawBitmap(Coords topLeft, TBitmapIn bitmap) {
      BitmapDrawingHelper::drawBitmap(_getGfxCanvas(), bitmap);
    }
};

class Canvas1 : public Canvas<GFXcanvas1, Bitmap1, 1> {
  public:
    Canvas1(const Coords dimensions) : Canvas<GFXcanvas1, Bitmap1, 1>(GFXcanvas1(dimensions.x, dimensions.y)) { }
};

class Canvas16 : Canvas<GFXcanvas16, Bitmap16, 16> {
  public:
    Canvas16(const Coords dimensions) : Canvas<GFXcanvas16, Bitmap16, 16>(GFXcanvas16(dimensions.x, dimensions.y)) { }
};

template <typename TDriver, typename TBitmap>
class AdafruitGfxDisplay : protected BitmapDrawingHelper, public Runnable {
  private:
    TDriver _driver;
    Input<TBitmap> _input;
  
  protected:
    TDriver* _getDriver() {
      return &_driver;
    }

    AdafruitGfxDisplay(TDriver driver, Input<TBitmap> input, Runner runner, Rotation rotation = deg_0)
    :
      _driver(driver),
      _input(input),
      Runnable(runner)
    {
      _driver.setRotation(rotation);
    }
  
  public:
    void run() {
      std::optional<TBitmap> bitmap = _input.read();
      if (!bitmap.has_value()) {
        return;
      }

      BitmapDrawingHelper::drawBitmap(_driver, Coords(0, 0), bitmap.value());
    }
};

template <typename TDriver>
class AdafruitGfxDisplayMonochrome : public AdafruitGfxDisplay<TDriver, DisplayBitmapMonochrome> {
  protected:
    AdafruitGfxDisplayMonochrome(TDriver driver, Input<DisplayBitmapMonochrome> input, Runner runner, Rotation rotation = deg_0)
    : AdafruitGfxDisplay<TDriver, DisplayBitmapMonochrome>(driver, input, runner, rotation)
    { }
};

template <typename TDriver>
class AdafruitGfxDisplayColour : public AdafruitGfxDisplay<TDriver, DisplayBitmapColour> {
  protected:
    AdafruitGfxDisplayColour(TDriver driver, Input<DisplayBitmapColour> input, Runner runner, Rotation rotation = deg_0)
    : AdafruitGfxDisplay<TDriver, DisplayBitmapColour>(driver, input, runner, rotation)
    { }
};

template <uint16_t W, uint16_t H, Rotation R>
class Ssd1306 : public AdafruitGfxDisplayMonochrome<Adafruit_SSD1306> {
  public:
    Ssd1306(Input<SizedDisplayBitmapMonochrome<W, H>> input, Runner runner, TwoWire wire, uint8_t rstPin = -1, Rotation rotation = deg_0)
    : 
      AdafruitGfxDisplayMonochrome<Adafruit_SSD1306>(
        Adafruit_SSD1306(
          rotationIsPerpendicular(R) ? H : W,
          rotationIsPerpendicular(R) ? W : H,
          &wire,
          rstPin
        ),
        input,
        runner,
        R
      )
    { }

    Ssd1306(Input<SizedDisplayBitmapMonochrome<W, H>> input, Runner runner, SPIClass spi, uint8_t dcPin, uint8_t csPin, uint8_t rstPin, Rotation rotation = deg_0)
    : 
      AdafruitGfxDisplayMonochrome<Adafruit_SSD1306>(
        Adafruit_SSD1306(
          rotationIsPerpendicular(R) ? H : W,
          rotationIsPerpendicular(R) ? W : H,
          &spi,
          dcPin,
          rstPin,
          csPin
        ),
        input,
        runner,
        R
      )
    { }
};

using Ssd1306_0_49_inches_landscape = Ssd1306<64, 32, deg_0>;
using Ssd1306_0_96_inches_landscape = Ssd1306<128, 64, deg_0>;

template <uint16_t W, uint16_t H, Rotation R>
class St7735 : public AdafruitGfxDisplayColour<Adafruit_ST7735> {
  public:
    St7735(Input<SizedDisplayBitmapColour<W, H>> input, Runner runner, SPIClass spi, uint8_t dcPin, uint8_t csPin, uint8_t rstPin, Rotation rotation = deg_0)
    :
      AdafruitGfxDisplayColour<Adafruit_ST7735>(
        Adafruit_ST7735(
          &spi,
          csPin,
          dcPin,
          rstPin
        ),
        input,
        runner,
        R
      )
    { }
};

using St7735_0_96_inches_landscape = St7735<160, 80, deg_0>;
using St7735_1_8_inches_portrait = St7735<128, 160, deg_0>;
using St7735_1_8_inches_landscape = St7735<160, 128, deg_90>;

template <uint16_t W, uint16_t H, Rotation R>
class Ili9341 : public AdafruitGfxDisplayColour<Adafruit_ILI9341> {
  public:
    Ili9341(Input<SizedDisplayBitmapColour<W, H>> input, Runner runner, SPIClass spi, uint8_t dcPin, uint8_t csPin, uint8_t rstPin = -1, Rotation rotation = deg_0)
    :
      AdafruitGfxDisplayColour<Adafruit_ILI9341>(
        Adafruit_ILI9341(
          &spi,
          dcPin,
          csPin,
          rstPin
        ),
        input,
        runner,
        R
      )
    { }
};

using Ili9341_2_8_inches_portrait = Ili9341<240, 320, deg_0>;
using Ili9341_2_8_inches_landscape = Ili9341<320, 240, deg_90>;

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
      canvas.drawText(Coords(0, 0), full);
      return canvas.getBitmap().withColours(FgBgColour(std::get<1>(textAndColour.value())));
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
        Coords(
          // If not given a space to fill, default to the size of the text.
          length.value_or(text.size()),
          // Labels are always one row tall. Sorry, but that's just the way it is.
          1
        ),
        fontSize,
        alignment
      )
    { }
};

template <typename T>
std::string formatNumber(T value, uint8_t base, uint8_t precision) {
  std::stringstream text;
  text << std::setbase(_base) << std::setprecision(_precision) << value.value() << std::endl;
  return text.str();
};

template <typename T>
class NumberToStringProcess :npublic TranslatingNotEmptyProcess<T, std::string> {
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
      std::map<T, Colour> breakpoints;
      for (auto i : breakpoints) {
        breakpoints[std::get<0>(i)] = std::get<1>(i);
      }
      return breakpoints;
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
    : BitmapBoxingProcess(Merging2NotEmptyProcess<TBitmapIn, FgBgColour>(bitmapInput, FgBgColour), cornerRadius, padding)
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
      image.fillRoundRect(Coords(0, 0), outerDimensions, _radius, colours.fgColour);
      if (colours.bgColour.has_value()) {
        image.drawRoundRect(Cords(0, 0), outerDimensions, _radius, colours.bgColour.value());
      }
      image.drawBitmap<TBitmapIn>(bitmap);

      Canvas1 mask(outerDimensions);
      mask.fillRoundRect(Coords(0, 0), outerDimensions, _radius, Colour(true));

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
class CompositorProcess : public Input<TSizedBitmapOut<W, H>> {
  private:
    std::vec<Input<PositionedBitmap<TDisplayBitmapIn>>> _inputs;

  public:
    CompositorProcess(std::vec<Input<PositionedBitmap<TDisplayBitmapIn>>> inputs)
    : _inputs(inputs)
    { }

    std::optional<TSizedBitmapOut<W, H>> read() {
      TCanvas canvas(W, H);

      for (auto input : _inputs) {
        std::optional<TPositionedBitmapIn<TDisplayBitmapIn>> value = _input.read();
        if (value.has_value()) {
          canvas.drawBitmap<TDisplayBitmapIn>(value.value().bitmap, value.value().topLeft);
        }
      }

      return SizedBitmap<W, H>(canvas.getBitmap());
    }
};

template <uint16_t W, uint16_t H>
using MonochromeCompositorProcess = CompositorProcess<DisplayBitmapMonochrome, SizedBitmap1, Canvas1, W, H>;

template <uint16_t W, uint16_t H>
using ColourCompositorProcess = CompositorProcess<DisplayBitmapColour, SizedBitmap16, Canvas16, W, H>;

using Ssd1306CompositorProcess_0_49_inches_landscape = MonochromeCompositorProcess<64, 32>;
using Ssd1306CompositorProcess_0_96_inches_landscape = MonochromeCompositorProcess<128, 64>;
using St7735CompositorProcess_0_96_inches_landscape = ColourCompositorProcess<160, 80>;
using St7735CompositorProcess_1_8_inches_portrait = ColourCompositorProcess<128, 160>;
using St7735CompositorProcess_1_8_inches_landscape = ColourCompositorProcess<160, 128>;
using Ili9341CompositorProcess_2_8_inches_portrait = ColourCompositorProcess<240, 320>;
using Ili9341CompositorProcess_2_8_inches_landscape = ColourCompositorProcess<320, 240>;

#endif