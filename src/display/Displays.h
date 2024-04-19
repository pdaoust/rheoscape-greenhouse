#ifndef RHEOSCAPE_DISPLAYS_H
#define RHEOSCAPE_DISPLAYS_H

#include <optional>
#include <vector>

#include <Adafruit_SSD1306.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_ILI9341.h>

#include <display/BitmapDrawingHelper.h>
#include <display/Canvas.h>
#include <input/Input.h>
#include <Runnable.h>

template <typename TDriver, typename TBitmap>
class AdafruitGfxDisplay : protected BitmapDrawingHelper, public Runnable {
  private:
    TDriver* _driver;
    Input<TBitmap>* _input;
  
  protected:
    TDriver _getDriver() {
      return _driver;
    }

    AdafruitGfxDisplay(TDriver* driver, Input<TBitmap>* input, Rotation rotation = deg_0)
    :
      _driver(driver),
      _input(input)
    {
      _driver.setRotation(rotation);
    }
  
  public:
    virtual void run() {
      TBitmap bitmap = _input->read();
      BitmapDrawingHelper::drawBitmap(_driver, Coords{0, 0}, bitmap);
    }
};

template <typename TDriver>
class AdafruitGfxDisplayMonochrome : public AdafruitGfxDisplay<TDriver, DisplayBitmapMonochrome> {
  protected:
    AdafruitGfxDisplayMonochrome(TDriver* driver, Input<DisplayBitmapMonochrome>* input, Rotation rotation = deg_0)
    : AdafruitGfxDisplay<TDriver, DisplayBitmapMonochrome>(driver, input, rotation)
    { }
};

template <typename TDriver>
class AdafruitGfxDisplayColour : public AdafruitGfxDisplay<TDriver, DisplayBitmapColour> {
  protected:
    AdafruitGfxDisplayColour(TDriver* driver, Input<DisplayBitmapColour>* input, Rotation rotation = deg_0)
    : AdafruitGfxDisplay<TDriver, DisplayBitmapColour>(driver, input, rotation)
    { }
};

template <uint16_t W, uint16_t H, Rotation R>
class Ssd1306 : public AdafruitGfxDisplayMonochrome<Adafruit_SSD1306> {
  public:
    Ssd1306(Input<SizedDisplayBitmapMonochrome<W, H>>* input, TwoWire wire, uint8_t rstPin = -1, Rotation rotation = deg_0)
    : 
      AdafruitGfxDisplayMonochrome<Adafruit_SSD1306>(
        // FIXME: memory leak
        new Adafruit_SSD1306(
          rotationIsPerpendicular(R) ? H : W,
          rotationIsPerpendicular(R) ? W : H,
          &wire,
          rstPin
        ),
        input,
        R
      )
    { }

    Ssd1306(Input<SizedDisplayBitmapMonochrome<W, H>>* input, SPIClass spi, uint8_t dcPin, uint8_t csPin, uint8_t rstPin, Rotation rotation = deg_0)
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
        R
      )
    { }
};

using Ssd1306_0_49_inches_landscape = Ssd1306<64, 32, deg_0>;
using Ssd1306_0_96_inches_landscape = Ssd1306<128, 64, deg_0>;

template <uint16_t W, uint16_t H, Rotation R>
class St7735 : public AdafruitGfxDisplayColour<Adafruit_ST7735> {
  public:
    St7735(Input<SizedDisplayBitmapColour<W, H>>* input, SPIClass spi, uint8_t dcPin, uint8_t csPin, uint8_t rstPin, Rotation rotation = deg_0)
    :
      AdafruitGfxDisplayColour<Adafruit_ST7735>(
        // FIXME: memory leak
        new Adafruit_ST7735(
          &spi,
          csPin,
          dcPin,
          rstPin
        ),
        input,
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
    Ili9341(Input<SizedDisplayBitmapColour<W, H>>* input, SPIClass spi, uint8_t dcPin, uint8_t csPin, uint8_t rstPin = -1, Rotation rotation = deg_0)
    :
      AdafruitGfxDisplayColour<Adafruit_ILI9341>(
        // FIXME: memory leak
        new Adafruit_ILI9341(
          &spi,
          dcPin,
          csPin,
          rstPin
        ),
        input,
        R
      )
    { }
};

using Ili9341_2_8_inches_portrait = Ili9341<240, 320, deg_0>;
using Ili9341_2_8_inches_landscape = Ili9341<320, 240, deg_90>;

#endif