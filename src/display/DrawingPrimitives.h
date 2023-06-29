#ifndef RHEOSCAPE_DRAWING_PRIMITIVES_H
#define RHEOSCAPE_DRAWING_PRIMITIVES_H

#include <Arduino.h>
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>

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

#endif