#ifndef RHEOSCAPE_CANVAS_H
#define RHEOSCAPE_CANVAS_H

#include <Adafruit_GFX.h>
#include <display/Bitmap.h>
#include <display/BitmapDrawingHelper.h>

template <typename TGfxCanvas, typename TBitmapOut, BitDepth BitDepth>
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
      _gfxCanvas.drawPixel(coords.x, coords.y, colour.toDisplayBitDepth(BitDepth));
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
      _gfxCanvas.drawFastVLine(topLeft.x, topLeft.y, height, colour.toDisplayBitDepth(BitDepth));
    }

    void drawFastHLine(Coords topLeft, int16_t length, Colour colour) {
      _gfxCanvas.drawFastHLine(topLeft.x, topLeft.y, length, colour.toDisplayBitDepth(BitDepth));
    }

    void fillRect(Coords corner1, Coords corner2, Colour colour) {
      _gfxCanvas.fillRect(corner1.x, corner1.y, corner2.x - corner1.x, corner2.y - corner1.y, colour.toDisplayBitDepth(BitDepth));
    }

    void fillScreen(Colour colour) {
      _gfxCanvas.fillScreen(colour.toDisplayBitDepth(BitDepth));
    }

    void drawLine(Coords start, Coords end, Colour colour) {
      _gfxCanvas.drawLine(start.x, start.y, end.x - start.x, end.y - start.y, colour.toDisplayBitDepth(BitDepth));
    }

    void drawRect(Coords corner1, Coords corner2, Colour colour) {
      _gfxCanvas.drawRect(corner1.x, corner1.y, corner2.x - corner1.x, corner2.y - corner1.y, colour.toDisplayBitDepth(BitDepth));
    }

    void drawCircle(Coords centre, int16_t radius, Colour colour) {
      _gfxCanvas.drawCircle(centre.x, centre.y, radius, colour.toDisplayBitDepth(BitDepth));
    }

    void fillCircle(Coords centre, int16_t radius, Colour colour) {
      _gfxCanvas.fillCircle(centre.x, centre.y, radius, colour.toDisplayBitDepth(BitDepth));
    }

    void drawTriangle(Coords point1, Coords point2, Coords point3, Colour colour) {
      _gfxCanvas.drawTriangle(point1.x, point1.y, point2.x, point2.y, point3.x, point3.y, colour.toDisplayBitDepth(BitDepth));
    }

    void fillTriangle(Coords point1, Coords point2, Coords point3, Colour colour) {
      _gfxCanvas.fillTriangle(point1.x, point1.y, point2.x, point2.y, point3.x, point3.y, colour.toDisplayBitDepth(BitDepth));
    }

    void drawRoundRect(Coords corner1, Coords corner2, int16_t radius, Colour colour) {
      _gfxCanvas.drawRoundRect(corner1.x, corner1.y, corner2.x - corner1.x, corner2.y - corner1.y, radius, colour.toDisplayBitDepth(BitDepth));
    }

    void fillRoundRect(Coords corner1, Coords corner2, int16_t radius, Colour colour) {
      _gfxCanvas.fillRoundRect(corner1.x, corner1.y, corner2.x - corner1.x, corner2.y - corner1.y, radius, colour.toDisplayBitDepth(BitDepth));
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
      canvas.print(text.formatTextWithClipAndAlignment().c_str());

      _gfxCanvas.drawBitmap(
        topLeft.x,
        topLeft.y,
        canvas.getBuffer(),
        canvas.width(),
        canvas.height(),
        // Every display I've seen so far is 16-bit only, so this should work.
        // FIXME: If I start using 24-bit colours, I'll have to revise this.
        text.colour.toDisplayBitDepth(BitDepth).to16Bit()
        // Never specify the background colour;
        // text is always transparent on its background.
        // The only value of specifying a background (that I can see)
        // is as a cheap crappy framebuffer for monospace text.
      );
    }

    template <typename TBitmapIn>
    void drawBitmap(Coords topLeft, TBitmapIn bitmap) {
      BitmapDrawingHelper::drawBitmap(_getGfxCanvas(), topLeft, bitmap);
    }
};

class Canvas1 : public Canvas<GFXcanvas1, Bitmap1, one> {
  public:
    Canvas1(const Coords dimensions) : Canvas<GFXcanvas1, Bitmap1, one>(GFXcanvas1(dimensions.x, dimensions.y)) { }
};

class Canvas16 : Canvas<GFXcanvas16, Bitmap16, sixteen> {
  public:
    Canvas16(const Coords dimensions) : Canvas<GFXcanvas16, Bitmap16, sixteen>(GFXcanvas16(dimensions.x, dimensions.y)) { }
};

#endif