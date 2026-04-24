#pragma once

#include "System/StringUtil.h"

static constexpr char8_t OldColorCodeIndicator = 0xFF; // ÿ
static constexpr char8_t OldColorCodeIndicatorEx = 0xFE; // þ
static constexpr char8_t ColorCodeIndicator = 0x11; // dc1
static constexpr char8_t ColorCodeIndicatorEx = 0x12; // dc2
static constexpr char8_t ColorResetIndicator = 0x08; // =: '\\b'

static constexpr char32_t spaceUTF16 = 0x20;
static constexpr char32_t ellipsisUTF16 = 0x2026;
static const std::string ellipsisUTF8 = utf8::FromUnicode(ellipsisUTF16);

static constexpr char8_t CR = '\r';
static constexpr char8_t LF = '\n';
static constexpr const char* CRLF = "\r\n";