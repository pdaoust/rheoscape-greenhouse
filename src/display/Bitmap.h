#ifndef RHEOSCAPE_BITMAP_H
#define RHEOSCAPE_BITMAP_H

#include <Arduino.h>
#include <optional>
#include <variant>

#include <display/DrawingPrimitives.h>

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
  : TBitmap(data, Coords{W, H}, mask)
  { }

  SizedBitmap(TBitmap unsized)
  : SizedBitmap(unsized.data, unsized.mask)
  { }
};

struct Bitmap1 : public Bitmap<uint8_t> {
  Bitmap1(const uint8_t* data, const Coords dimensions, const std::optional<uint8_t*> mask = std::nullopt)
  : Bitmap<uint8_t>(data, dimensions, mask)
  { }
};

template <uint16_t W, uint16_t H>
using SizedBitmap1 = SizedBitmap<W, H, uint8_t, Bitmap1>;

using Bitmap16 = Bitmap<uint16_t>;

template <uint16_t W, uint16_t H>
using SizedBitmap16 = SizedBitmap<W, H, uint16_t, Bitmap16>;

struct ColouredBitmap1 : public Bitmap1 {
  const FgBgColour colours;

  ColouredBitmap1(const uint8_t* data, const Coords dimensions, const FgBgColour colours, const std::optional<uint8_t*> mask = std::nullopt)
  :
    Bitmap1(data, dimensions, mask),
    colours(colours)
  { }

  static ColouredBitmap1 fromBitmap1(Bitmap1 bitmap, FgBgColour colours) {
    return ColouredBitmap1(
      bitmap.data,
      bitmap.dimensions,
      colours,
      bitmap.mask
    );
  }
};

template <uint16_t W, uint16_t H>
struct SizedColouredBitmap1 : public SizedBitmap<W, H, uint8_t, ColouredBitmap1> {
  SizedColouredBitmap1(const uint8_t* data, const FgBgColour colours, const std::optional<uint8_t*> mask = std::nullopt)
  : ColouredBitmap1(data, Coords{W, H}, colours, mask)
  { }
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


#endif