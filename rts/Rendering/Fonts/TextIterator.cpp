#include "TextIterator.hpp"

#include "FontHandler.h"
#include "FontConstants.hpp"
#include "System/StringUtil.h"
#include "System/Misc/TracyDefs.h"
#include "System/TemplateUtils.hpp"

void TextIterator::Execute()
{
    RECOIL_DETAILED_TRACY_ZONE;
    const int end = static_cast<int>(text.length());

    while (currentPos < end) {
        const int startPos = currentPos;
        const char32_t ch = utf8::GetNextChar(text, currentPos);

        CharEvent event {
            .startIdx = startPos,
            .endIdx = currentPos,
            .linesSkipped = 0,
            .value = ch
        };

        switch (ch) {

        case OldColorCodeIndicator: {
            if (fontHandler.disableOldColorIndicators) {
                // Treat as printable character
                if (!handler.OnPrintable(event))
                    return;
                break;
            }
            [[fallthrough]];
        }
        case ColorCodeIndicator: {
            event.endIdx = std::min(currentPos + 3, end); // 3 more bytes: R, G, B
            currentPos = event.endIdx;
            if (ExtractColor(event) && !handler.OnColorCode(event))
                return;
        } break;

        case OldColorCodeIndicatorEx: {
            if (fontHandler.disableOldColorIndicators) {
                // Treat as printable character
                if (!handler.OnPrintable(event))
                    return;
                break;
            }
            [[fallthrough]];
        }
        case ColorCodeIndicatorEx: {
            event.endIdx = std::min(currentPos + 8, end); // 8 more bytes: R,G,B,A,R,G,B,A
            currentPos = event.endIdx;
            if (ExtractColorEx(event) && !handler.OnColorCodeEx(event))
                return;
        } break;

        case ColorResetIndicator: {
            // Already advanced by GetNextChar
            if (!handler.OnColorReset(event))
                return;
        } break;

        case CR: {
            // Check for CRLF sequence
            if (currentPos < end && text[currentPos] == LF) {
                currentPos++; // Skip the LF
            }
            [[fallthrough]];
        }
        case LF: {
            skippedLines++;
            event.linesSkipped = skippedLines;
            if (!handler.OnNewline(event)) {
                skippedLines = 0;
                return;
            }
            skippedLines = 0; // Reset after processing
        } break;

        case spaceUTF16: {
            // Already advanced by GetNextChar
            if (!handler.OnSpace(event))
                return;
        } break;

        default: {
            // Printable character - already advanced
            if (!handler.OnPrintable(event))
                return;
        } break;
        }
    }

    handler.OnEnd();
}

bool TextIterator::ExtractColor(CharEvent& e) const
{
    if (e.endIdx - e.startIdx != 1 + 3) {
        return false;
    }

    e.value = SColor {
        static_cast<uint8_t>(text[e.endIdx - 3]),
        static_cast<uint8_t>(text[e.endIdx - 2]),
        static_cast<uint8_t>(text[e.endIdx - 1]),
        static_cast<uint8_t>(255)
    };
    return true;
}
bool TextIterator::ExtractColorEx(CharEvent& e) const
{
    if (e.endIdx - e.startIdx != 1 + 4 + 4) {
        return false;
    }

    e.value = FontColors {
        .textColor = SColor {
            static_cast<uint8_t>(text[e.endIdx - 4]),
            static_cast<uint8_t>(text[e.endIdx - 3]),
            static_cast<uint8_t>(text[e.endIdx - 2]),
            static_cast<uint8_t>(text[e.endIdx - 1])
        },
        .outlineColor = SColor {
            static_cast<uint8_t>(text[e.endIdx - 8]),
            static_cast<uint8_t>(text[e.endIdx - 7]),
            static_cast<uint8_t>(text[e.endIdx - 6]),
            static_cast<uint8_t>(text[e.endIdx - 5])
        }
    };
    return true;
}

std::string CharVariantToString(const CharVariant& cv)
{
    return std::visit([](const auto& v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        std::string res;
        if constexpr (std::is_same_v<T, SColor>) {
            res.resize(1 + 3);
            res[0] = fontHandler.disableOldColorIndicators ? ColorCodeIndicator : OldColorCodeIndicator;
            res[1] = v.r;
            res[2] = v.g;
            res[3] = v.b;
        }
        else if constexpr (std::is_same_v<T, FontColors>) {
            res.resize(1 + 4 + 4);
            res[0] = fontHandler.disableOldColorIndicators ? ColorCodeIndicatorEx : OldColorCodeIndicatorEx;
            res[1] = v.textColor.r;
            res[2] = v.textColor.g;
            res[3] = v.textColor.b;
            res[4] = v.textColor.a;
            res[5] = v.outlineColor.r;
            res[6] = v.outlineColor.g;
            res[7] = v.outlineColor.b;
            res[8] = v.outlineColor.a;
        }
        else if constexpr (std::is_same_v<T, char32_t>) {
            return utf8::FromUnicode(v);
        }
        else {
            static_assert(Recoil::always_false_v<T>, "Unhandled visited type");
        }

        return res;
    }, cv);
}
