/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _TEXTWRAP_H
#define _TEXTWRAP_H

#include <string>
#include <list>
#include <array>

#include "FontHandler.h"
#include "CFontTexture.h"
#include "ustring.h"
#include "System/Color.h"

class CTextWrap : public CFontTexture
{
public:
	//! Adds \n's (and '...' if it would be too high) until the text fits into maxWidth/maxHeight
	inline int WrapInPlace(std::string& text, float fontSize,  float maxWidth, float maxHeight = MAX_HEIGHT_DEFAULT);
	inline std::string Wrap(const std::string& text, float fontSize, float maxWidth, float maxHeight = MAX_HEIGHT_DEFAULT);

	static constexpr float MAX_HEIGHT_DEFAULT = 1e3;

	static constexpr char8_t OldColorCodeIndicator   = 0xFF; // ÿ
	static constexpr char8_t OldColorCodeIndicatorEx = 0xFE; // þ
	static constexpr char8_t ColorCodeIndicator   = 0x11; // dc1
	static constexpr char8_t ColorCodeIndicatorEx = 0x12; // dc2
	static constexpr char8_t ColorResetIndicator  = 0x08; // =: '\\b'

	static constexpr char8_t CR = '\r';
	static constexpr char8_t LF = '\n';
public:
	struct ColorCodeText {
		std::array<char8_t, 1 + 4 + 4 + 1> storage; // reserve for [I,R,G,B,A,R,G,B,A] + \0
		uint8_t size;
	};
	struct ColorCode {
		auto tostring() const { return std::string(reinterpret_cast<const char*>(colorText.storage.data()), colorText.size); }
		ColorCodeText colorText;
		uint32_t pos;
	};
protected:
	CTextWrap(const std::string& fontfile, int size, int outlinesize, float  outlineweight);
	virtual ~CTextWrap() {}
protected:
	uint32_t SkipColorCodes(const spring::u8string& text, uint32_t idx, ColorCodeText* cctPtr = nullptr);
public:
	virtual float GetTextWidth(const std::string& text) = 0;
	virtual void ScanForWantedGlyphs(const spring::u8string& str) = 0;
private:
	struct word {
		word() : width(0.0f), text(""), isSpace(false), isLineBreak(false), isColorCode(false), numSpaces(0), pos(0) {};

		float width;
		spring::u8string text;
		bool isSpace;
		bool isLineBreak;
		bool isColorCode;
		unsigned int numSpaces;
		unsigned int pos; //! position in the original text (needed for remerging colorcodes after wrapping; in printable chars (linebreaks and space don't count))
	};
	struct line {
		line() : width(0.0f), cost(0.0f), forceLineBreak(false) {};

		std::list<word>::iterator start, end;
		float width;
		float cost;
		bool forceLineBreak;
	};

	word SplitWord(word& w, float wantedWidth, bool smart = true);

	void SplitTextInWords(const spring::u8string& text, std::list<word>* words, std::list<ColorCode>& colorCodes);
	void RemergeColorCodes(std::list<word>* words, const std::list<ColorCode>& colorCodes) const;

	void AddEllipsis(std::list<line>& lines, std::list<word>& words, float maxWidth);

	void WrapTextConsole(std::list<word>& words, float maxWidth, float maxHeight);

	int WrapInPlace(spring::u8string& text, float fontSize,  float maxWidth, float maxHeight = MAX_HEIGHT_DEFAULT);
	spring::u8string Wrap(const spring::u8string& text, float fontSize, float maxWidth, float maxHeight = MAX_HEIGHT_DEFAULT);
};

// wrappers
int CTextWrap::WrapInPlace(std::string& text, float fontSize,  float maxWidth, float maxHeight)
{
	return WrapInPlace(toustring(text), fontSize, maxWidth, maxHeight);
}
std::string CTextWrap::Wrap(const std::string& text, float fontSize, float maxWidth, float maxHeight)
{
	return Wrap(toustring(text), fontSize, maxWidth, maxHeight);
}

#endif /* _TEXTWRAP_H */
