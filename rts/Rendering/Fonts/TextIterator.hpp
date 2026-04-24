#pragma once

#include "ustring.h"
#include <variant>
#include <optional>
#include <array>
#include "System/Color.h"

struct FontColors {
    SColor textColor;
    SColor outlineColor;
};

using CharVariant = std::variant<char32_t, SColor, FontColors>;
std::string CharVariantToString(const CharVariant& cv);

struct CharEvent {
    int startIdx;
    int endIdx;
    int linesSkipped;

    CharVariant value;
    inline std::string ToString() const { return CharVariantToString(value); }
};

struct TextIteratorHandler {
    virtual bool OnColorCode(const CharEvent& e) { return true; }
    virtual bool OnColorCodeEx(const CharEvent& e) { return true; }
    virtual bool OnColorReset(const CharEvent& e) { return true; }
    virtual bool OnNewline(const CharEvent& e) { return true; }
    virtual bool OnPrintable(const CharEvent& e) { return true; }
    virtual bool OnSpace(const CharEvent& e) { return true; }
    virtual void OnEnd() {}
};

class TextIterator {
private:
    const spring::u8string& text;
    TextIteratorHandler& handler;
    int currentPos = 0;
    int skippedLines = 0;
public:
    TextIterator(const spring::u8string& text, TextIteratorHandler& handler)
        : text(text)
        , handler(handler)
    {}

    void Execute();
private:
    bool ExtractColor(CharEvent& e) const;
    bool ExtractColorEx(CharEvent& e) const;
};