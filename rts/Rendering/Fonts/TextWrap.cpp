/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cstdint>

#include "TextWrap.h"
#include "glFont.h"
#include "FontLogSection.h"
#include "FontConstants.hpp"
#include "TextIterator.hpp"
#include "System/Log/ILog.h"
#include "System/SpringMath.h"
#include "System/StringUtil.h"

#include "System/Misc/TracyDefs.h"



static constexpr const char* spaceStringTable[1 + 10] = {
	"",
	" ",
	"  ",
	"   ",
	"    ",
	"     ",
	"      ",
	"       ",
	"        ",
	"         ",
	"          ",
};

/*******************************************************************************/
/*******************************************************************************/

CTextWrap::CTextWrap(const std::string& fontfile, int size, int outlinewidth, float  outlineweight)
: CFontTexture(fontfile,size,outlinewidth,outlineweight)
{
}


/*******************************************************************************/
/*******************************************************************************/

/**
 * @brief IsUpperCase
 * @return true if the given char is an uppercase
 */
static inline bool IsUpperCase(const char32_t& c)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// overkill to add unicode
	return
		(c >= 0x41 && c <= 0x5A) ||
		(c >= 0xC0 && c <= 0xD6) ||
		(c >= 0xD8 && c <= 0xDE) ||
		(c == 0x8A) ||
		(c == 0x8C) ||
		(c == 0x8E) ||
		(c == 0x9F);
}

static inline bool IsLowerCase(const char32_t& c)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// overkill to add unicode
	return c >= 0x61 && c <= 0x7A; // only ascii (no latin-1!)
}


/**
 * @brief GetPenalty
 * @param c character at %strpos% in the word
 * @param strpos position of c in the word
 * @param strlen total length of the word
 * @return penalty (smaller is better) to split a word at that position
 */
static inline float GetPenalty(const char32_t& c, uint32_t strpos, uint32_t strlen)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const float dist = strlen - strpos;

	if (dist > (strlen / 2) && dist < 4) {
		return 1e9;
	} else if (IsLowerCase(c)) {
		// lowercase char
		return 1.0f + (strlen - strpos);
	} else if (c >= 0x30 && c <= 0x39) {
		// is number
		return 1.0f + (strlen - strpos)*0.9;
	} else if (IsUpperCase(c)) {
		// uppercase char
		return 1.0f + (strlen - strpos)*0.75;
	}

	// any special chars
	return Square(dist / 4);
}


TextWrapHelpers::word CTextWrap::SplitWord(TextWrapHelpers::word& w, float wantedWidth, bool smart)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// returns two pieces 'L'eft and 'R'ight of the split word (returns L, *wi becomes R)

	word w2;
	w2.pos = w.pos;

	const float spaceAdvance = GetGlyph(spaceUTF16).advance;
	if (w.isLineBreak) {
		// shouldn't happen
		w2 = w;
		w.isSpace = true;
	} else if (w.isSpace) {
		const int split = (int)std::floor(wantedWidth / spaceAdvance);
		w2.isSpace   = true;
		w2.numSpaces = split;
		w2.width     = spaceAdvance * w2.numSpaces;
		w.numSpaces -= split;
		w.width      = spaceAdvance * w.numSpaces;
		w.pos       += split;
	} else {
		if (smart) {
			if (
				(wantedWidth < 8 * spaceAdvance) ||
				(w.text.length() < 1)
			) {
				w2.isSpace = true;
				return w2;
			}
		}

		// Use TextIterator to iterate through the word
		struct SplitWordHandler : public TextIteratorHandler {
			CTextWrap* font;
			float wantedWidth;
			bool smart;
			float width = 0.0f;
			float minPenalty = 1e9f;
			uint32_t goodbreak = 0;
			const GlyphInfo* prevGlyph = nullptr;
			int textLen;

			SplitWordHandler(CTextWrap* f, float w, bool s, int len)
				: font(f), wantedWidth(w), smart(s), textLen(len) {}

			bool OnPrintable(const CharEvent& e) override {
				char32_t c = std::get<char32_t>(e.value);
				const GlyphInfo* curGlyph = &font->GetGlyph(c);
				if (prevGlyph != nullptr) {
					width += font->GetKerning(*prevGlyph, *curGlyph);
				}
				if (width > wantedWidth) {
					return false; // stop further processing
				}
				if (smart) {
					float penalty = GetPenalty(c, e.startIdx, static_cast<uint32_t>(textLen));
					if (penalty < minPenalty) {
						minPenalty = penalty;
						goodbreak = e.startIdx;
					}
				} else {
					goodbreak = e.endIdx;
				}
				prevGlyph = curGlyph;
				return true;
			}
			bool OnSpace(const CharEvent& e) override {
				return OnPrintable(e); // treat spaces like printable
			}
		};

		SplitWordHandler handler(this, wantedWidth, smart, w.text.length());
		TextIterator iterator(w.text, handler);
		iterator.Execute();

		uint32_t goodbreak = handler.goodbreak;

		w2.text  = toustring(w.text.substr(0,goodbreak));
		w2.width = GetTextWidth(w2.text);
		w.text.erase(0,goodbreak);
		w.width  = GetTextWidth(w.text);
		w.pos   += goodbreak;
	}
	return w2;
}


void CTextWrap::AddEllipsis(std::list<line>& lines, std::list<word>& words, float maxWidth)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const float ellipsisAdvance = GetGlyph(ellipsisUTF16).advance;
	const float spaceAdvance = GetGlyph(spaceUTF16).advance;

	if (ellipsisAdvance > maxWidth)
		return;

	line* l = &(lines.back());

	// If the last line ends with a linebreak, remove it
	std::list<word>::iterator wi_end = l->end;
	if (wi_end->isLineBreak) {
		if (l->start == l->end || l->end == words.begin()) {
			// there is just the linebreak in that line, so replace linebreak with just a null space
			word w;
			w.pos       = wi_end->pos;
			w.isSpace   = true;
			w.numSpaces = 0;
			l->start = words.insert(wi_end,w);
			l->end = l->start;

			words.erase(wi_end);
		} else {
			wi_end = words.erase(wi_end);
			l->end = --wi_end;
		}
	}

	// remove as many words until we have enough free space for the ellipsis
	while (l->end != l->start) {
		word& w = *l->end;

		// we have enough free space
		if (l->width + ellipsisAdvance < maxWidth)
			break;

		// we can cut the last word to get enough freespace (so show as many as possible characters of that word)
		if (
			((l->width - w.width + ellipsisAdvance) < maxWidth) &&
			(w.width > ellipsisAdvance)
		) {
			break;
		}

		l->width -= w.width;
		--(l->end);
	}

	// we don't even have enough space for the ellipsis
	word& w = *l->end;
	if ((l->width - w.width) + ellipsisAdvance > maxWidth)
		return;

	// sometimes words aren't hyphenated for visual aspects
	// but if we put an ellipsis in there, it is better to show as many as possible characters of those words
	std::list<word>::iterator nextwi(l->end);
	++nextwi;
	if (
		(!l->forceLineBreak) &&
		(nextwi != words.end()) &&
		(w.isSpace || w.isLineBreak) &&
		(l->width + ellipsisAdvance < maxWidth) &&
		!(nextwi->isSpace || nextwi->isLineBreak)
	) {
		float spaceLeft = maxWidth - (l->width + ellipsisAdvance);
		l->end = words.insert( nextwi, SplitWord(*nextwi, spaceLeft, false) );
		l->width += l->end->width;
	}

	// the last word in the line needs to be cut
	if (l->width + ellipsisAdvance > maxWidth) {
		word& w = *l->end;
		l->width -= w.width;
		float spaceLeft = maxWidth - (l->width + ellipsisAdvance);
		l->end = words.insert( l->end, SplitWord(w, spaceLeft, false) );
		l->width += l->end->width;
	}

	// put in a space between words and the ellipsis (if there is enough space)
	if (l->forceLineBreak && !l->end->isSpace) {
		if (l->width + ellipsisAdvance + spaceAdvance <= maxWidth) {
			word space;
			space.isSpace = true;
			space.numSpaces = 1;
			space.width = spaceAdvance;
			std::list<word>::iterator wi(l->end);
			++l->end;
			if (l->end == words.end()) {
				space.pos = wi->pos + wi->text.length() + 1;
			} else {
				space.pos = l->end->pos;
			}
			l->end = words.insert( l->end, space );
			l->width += l->end->width;
		}
	}

	// add our ellipsis
	word ellipsis;
	ellipsis.text  = toustring(ellipsisUTF8);
	ellipsis.width = ellipsisAdvance;
	std::list<word>::iterator wi(l->end);
	++l->end;
	if (l->end == words.end()) {
		ellipsis.pos = wi->pos + wi->text.length() + 1;
	} else {
		ellipsis.pos = l->end->pos;
	}
	l->end = words.insert( l->end, ellipsis );
	l->width += l->end->width;
}


void CTextWrap::WrapTextConsole(std::list<word>& words, float maxWidth, float maxHeight)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (words.empty() || (GetLineHeight()<=0.0f))
		return;
	const bool splitAllWords = false;
	const uint32_t maxLines = (uint32_t)std::floor(std::max(0.0f, maxHeight / GetLineHeight()));

	line* currLine;
	word linebreak;
	linebreak.isLineBreak = true;

	bool addEllipsis = false;
	bool currLineValid = false; // true if there was added any data to the current line

	std::list<word>::iterator wi = words.begin();

	std::list<line> lines;
	lines.emplace_back();
	currLine = &(lines.back());
	currLine->start = words.begin();

	for (;;) {
		currLineValid = true;
		if (wi->isLineBreak) {
			currLine->forceLineBreak = true;
			currLine->end = wi;

			// start a new line after the '\n'
			lines.emplace_back();
			currLineValid = false;
			currLine = &(lines.back());
			currLine->start = wi;
			++currLine->start;
		} else {
			currLine->width += wi->width;
			currLine->end = wi;

			if (currLine->width > maxWidth) {
				currLine->width -= wi->width;

				// line grew too long by adding the last word, insert a LineBreak
				const bool splitLastWord = (wi->width > (0.5 * maxWidth));
				const float freeWordSpace = (maxWidth - currLine->width);

				if (splitAllWords || splitLastWord) {
					// last word W is larger than 0.5 * maxLineWidth, split it into
					// get 'L'eft and 'R'ight parts of the split (wL becomes Left, *wi becomes R)

					bool restart = (currLine->start == wi);
					// turns *wi into R
					word wL = SplitWord(*wi, freeWordSpace);

					if (splitLastWord && wL.width == 0.0f) {
						// With smart splitting it can happen that the word isn't split at all,
						// this can cause a race condition when the word is longer than maxWidth.
						// In this case we have to force an unaesthetic split.
						wL = SplitWord(*wi, freeWordSpace, false);
					}

					// increase by the width of the L-part of *wi
					currLine->width += wL.width;

					// insert the L-part right before R
					wi = words.insert(wi, wL);
					if (restart)
						currLine->start = wi;
					++wi;
				}

				// insert the forced linebreak (either after W or before R)
				linebreak.pos = wi->pos;
				currLine->end = words.insert(wi, linebreak);

				while (wi != words.end() && wi->isSpace)
					wi = words.erase(wi);

				lines.emplace_back();
				currLineValid = false;
				currLine = &(lines.back());
				currLine->start = wi;
				--wi; // compensate the wi++ downwards
			}
		}

		++wi;

		if (wi == words.end()) {
			break;
		}

		if (lines.size() > maxLines) {
			addEllipsis = true;
			break;
		}
	}

	// empty row
	if (!currLineValid || (currLine->start == words.end() && !currLine->forceLineBreak)) {
		lines.pop_back();
		currLine = &(lines.back());
	}

	// if we had to cut the text because of missing space, add an ellipsis
	if (addEllipsis)
		AddEllipsis(lines, words, maxWidth);

	wi = currLine->end;
	++wi;
	wi = words.erase(wi, words.end());
}

struct SplitWordsHandler final : TextIteratorHandler {
	CTextWrap& wrap;
	const spring::u8string& text;

	std::list<word>& words;
	std::list<ColorCode>& colorCodes;

	const float spaceAdvance;
	word* w = nullptr;
	uint32_t numChar = 0;

	SplitWordsHandler(CTextWrap& wrap,
		const spring::u8string& text,
		std::list<word>& words,
		std::list<ColorCode>& colorCodes,
		float spaceAdvance)
		: wrap(wrap)
		, text(text)
		, words(words)
		, colorCodes(colorCodes)
		, spaceAdvance(spaceAdvance)
	{
		words.emplace_back();
		w = &words.back();
	}

	void FinalizeCurrentWord()
	{
		if (!w) return;
		if (w->isSpace) {
			w->width = spaceAdvance * w->numSpaces;
		}
		else if (!w->isLineBreak) {
			w->width = wrap.GetTextWidth(w->text);
		}
	}

	void StartWord(uint32_t pos, bool isSpace, bool isLineBreak)
	{
		words.emplace_back();
		w = &words.back();
		w->pos = pos;
		w->isSpace = isSpace;
		w->isLineBreak = isLineBreak;
	}

	bool OnColorCode(const CharEvent& e) override
	{
		const auto& col = std::get<SColor>(e.value);
		ColorCode cc;
		cc.pos = numChar;
		cc.colorText = col;
		colorCodes.emplace_back(cc);

		return true;
	}

	bool OnColorCodeEx(const CharEvent& e) override
	{
		const auto& fc = std::get<FontColors>(e.value);
		ColorCode cc;
		cc.pos = numChar;
		cc.colorText = fc.textColor;

		colorCodes.emplace_back(cc);
		return true;
	}

	bool OnSpace(const CharEvent&) override
	{
		if (!w->isSpace) {
			FinalizeCurrentWord();
			StartWord(numChar, /*isSpace=*/true, /*isLineBreak=*/false);
		}
		w->numSpaces++;
		w->width = spaceAdvance * w->numSpaces;
		return true;
	}

	bool OnNewline(const CharEvent&) override
	{
		FinalizeCurrentWord();
		StartWord(numChar, /*isSpace=*/false, /*isLineBreak=*/true);
		return true;
	}

	bool OnPrintable(const CharEvent& e) override
	{
		if (w->isSpace || w->isLineBreak) {
			FinalizeCurrentWord();
			StartWord(numChar, /*isSpace=*/false, /*isLineBreak=*/false);
		}

		// Append original UTF-8 bytes for this codepoint:
		w->text.append(text.begin() + e.startIdx, text.begin() + e.endIdx);

		// only increment on printable (not on spaces/newlines)
		numChar++;
		return true;
	}

	void OnEnd() override
	{
		FinalizeCurrentWord();
	}
};

void CTextWrap::SplitTextInWords(const spring::u8string& text,
	std::list<word>* words,
	std::list<ColorCode>& colorCodes)
{
	RECOIL_DETAILED_TRACY_ZONE;

	ScanForWantedGlyphs(text);
	const float spaceAdvance = GetGlyph(spaceUTF16).advance;

	SplitWordsHandler h(*this, text, *words, colorCodes, spaceAdvance);
	TextIterator it(text, h);
	it.Execute();
}

void CTextWrap::RemergeColorCodes(std::list<word>* words, const std::list<ColorCode>& colorCodes) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto wi = words->begin();
	auto wi2 = words->begin();

	for (const auto& c: colorCodes) {
		while(wi != words->end() && wi->pos <= c.pos) {
			wi2 = wi;
			++wi;
		}

		word wc;
		wc.pos = c.pos;
		wc.isColorCode = true;
		wc.text = toustring(c.tostring());

		if (wi2->isSpace || wi2->isLineBreak) {
			while(wi2 != words->end() && (wi2->isSpace || wi2->isLineBreak))
				++wi2;

			if (wi2 == words->end() || (wi == words->end() && (wi2->pos + wi2->numSpaces) < c.pos))
				return;

			wi2 = words->insert(wi2, wc);
		} else {
			if (wi == words->end() && (wi2->pos + wi2->text.size()) < (c.pos + 1)) {
				return;
			}

			size_t pos = c.pos - wi2->pos;
			if (pos < wi2->text.size() && pos > 0) {
				word w2;
				w2.text = toustring(wi2->text.substr(0,pos));
				w2.pos = wi2->pos;
				wi2->text.erase(0,pos);
				wi2->pos += pos;
				wi2 = words->insert(wi2, wc);
				wi2 = words->insert(wi2, w2);
			} else {
				wi2 = words->insert(wi2, wc);
			}
		}
		wi = wi2;
		++wi;
	}
}


int CTextWrap::WrapInPlace(spring::u8string& text, float _fontSize, float maxWidth, float maxHeight)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// TODO make an option to insert '-' for word wrappings (and perhaps try to syllabificate)

	if (_fontSize <= 0.0f)
		_fontSize = GetSize();

	maxWidth = std::max(maxWidth, 10.0f); //otherwise endless loop with OOM might happen

	const float maxWidthf  = maxWidth / _fontSize;
	const float maxHeightf = maxHeight / _fontSize;

	// includes the empty string
	constexpr size_t numSpaceStrings = sizeof(spaceStringTable) / sizeof(spaceStringTable[0]);

	std::list<word> words;
	std::list<ColorCode> colorCodes;

	SplitTextInWords(text, &words, colorCodes);
	WrapTextConsole(words, maxWidthf, maxHeightf);
	//WrapTextKnuth(&lines, words, maxWidthf, maxHeightf);
	RemergeColorCodes(&words, colorCodes);

	// create the wrapped string
	text.clear();
	text.reserve(words.size());

	if (words.empty())
		return 0;

	uint32_t numlines = 1;

	for (const auto& w: words) {
		if (w.isSpace) {
			if (w.numSpaces < numSpaceStrings) {
				text.append(spaceStringTable[w.numSpaces]);
			} else {
				text.append(spaceStringTable[numSpaceStrings - 1]);
				text.append(w.numSpaces - (numSpaceStrings - 1), ' ');
			}
		} else if (w.isLineBreak) {
			text.append(CRLF);
			numlines++;
		} else {
			text.append(w.text);
		}
	}

	return numlines;
}


spring::u8string CTextWrap::Wrap(const spring::u8string& text, float _fontSize, float maxWidth, float maxHeight)
{
	RECOIL_DETAILED_TRACY_ZONE;
	spring::u8string out(text);
	WrapInPlace(out, _fontSize, maxWidth, maxHeight);
	return out;
}

/*******************************************************************************/
/*******************************************************************************/

std::string TextWrapHelpers::ColorCode::tostring() const
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

		return res;
	}, colorText);
}
