#ifndef RHEOSCAPE_BITMAP_DRAWING_HELPER_H
#define RHEOSCAPE_BITMAP_DRAWING_HELPER_H

#include <Adafruit_GFX.h>
#include <display/Bitmap.h>

class BitmapDrawingHelper {
  private:
    static void _drawBitmapWithoutMask(Adafruit_GFX* driver, Coords topLeft, Bitmap1 bitmap) {
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
    static void _drawBitmapWithoutMask(Adafruit_GFX* driver, Coords topLeft, ColouredBitmap1 bitmap) {
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

    static void _drawBitmapWithoutMask(Adafruit_GFX* driver, Coords topLeft, Bitmap16 bitmap) {
      driver->drawRGBBitmap(
        topLeft.x,
        topLeft.y,
        bitmap.data,
        bitmap.dimensions.x,
        bitmap.dimensions.y
      );
    }

    template <typename TBitmap>
    static void _drawBitmapWithMask(Adafruit_GFX* driver, Coords topLeft, TBitmap bitmap) {
      // Regrdless of the bit depth of the underlying driver or the bitmap to be drawn,
      // draw the maskless bitmap onto a colour canvas.
      // This is because the only bitmap that can have a mask is a colour one.
      GFXcanvas16 colourCanvas(bitmap.dimensions.x, bitmap.dimensions.y);
      _drawBitmapWithoutMask(&colourCanvas, Coords{0, 0}, bitmap);

      // Once we've done that, draw it to the driver with the mask.
      // If the bitmap is monochrome and the driver is monochrome,
      // 0xFFFF will get turned into 1.
      driver->drawRGBBitmap(
        topLeft.x,
        topLeft.y,
        colourCanvas.getBuffer(),
        bitmap.mask.value(),
        bitmap.dimensions.x,
        bitmap.dimensions.y
      );
    }

  public:
    template <typename TBitmap>
    static void drawBitmap(Adafruit_GFX* driver, Coords topLeft, TBitmap bitmap) {
      if (bitmap.mask.has_value()) {
        _drawBitmapWithMask<TBitmap>(driver, topLeft, bitmap);
      } else {
        _drawBitmapWithoutMask(driver, topLeft, bitmap);
      }
    }

    static void drawBitmap(Adafruit_GFX* driver, Coords topLeft, DisplayBitmapMonochrome bitmap) {
      if (std::holds_alternative<Bitmap1>(bitmap)) {
        Bitmap1 inner = std::get<Bitmap1>(bitmap);
        drawBitmap(driver, topLeft, inner);
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

#endif