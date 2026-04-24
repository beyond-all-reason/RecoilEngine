/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "glFont.h"
#include "glFontRenderer.h"
#include "FontLogSection.h"
#include "FontHandler.h"
#include "FontConstants.hpp"
#include "TextIterator.hpp"

#include <cstdarg>
#include <stdexcept>

#include "Game/Camera.h"
#include "Rendering/GlobalRendering.h"
#include "System/Color.h"
#include "System/Exceptions.h"
#include "System/SpringMath.h"
#include "System/SafeUtil.h"
#include "System/StringUtil.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"

#include "System/Misc/TracyDefs.h"

#undef GetCharWidth // winapi.h

CONFIG(std::string,      FontFile).defaultValue("fonts/FreeSansBold.otf").description("Sets the font of Spring engine text.");
CONFIG(std::string, SmallFontFile).defaultValue("fonts/FreeSansBold.otf").description("Sets the font of Spring engine small text.");

CONFIG(int,      FontSize).defaultValue(23).description("Sets the font size (in pixels) of the MainMenu and more.");
CONFIG(int, SmallFontSize).defaultValue(14).description("Sets the font size (in pixels) of the engine GUIs and more.");
CONFIG(int,      FontOutlineWidth).defaultValue(2).description("Sets the width of the black outline around Spring engine text, such as the title screen version number, clock, and basic UI. Does not affect LuaUI elements.");
CONFIG(int, SmallFontOutlineWidth).defaultValue(2).description("see FontOutlineWidth");
CONFIG(float,      FontOutlineWeight).defaultValue(25.0f).description("Sets the opacity of Spring engine text, such as the title screen version number, clock, and basic UI. Does not affect LuaUI elements.");
CONFIG(float, SmallFontOutlineWeight).defaultValue(10.0f).description("see FontOutlineWeight");
CONFIG(bool, AllowColorFonts).defaultValue(false).description("Allow working with colored fonts (experimental).");

std::shared_ptr<CglFont> font = nullptr;
std::shared_ptr<CglFont> smallFont = nullptr;

static constexpr SColor        white(1.00f, 1.00f, 1.00f, 0.95f);
static constexpr SColor  darkOutline(0.05f, 0.05f, 0.05f, 0.95f);
static constexpr SColor lightOutline(0.95f, 0.95f, 0.95f, 0.80f);

static const float darkLuminosity = 0.05f +
	0.2126f * std::pow(darkOutline[0], 2.2f) +
	0.7152f * std::pow(darkOutline[1], 2.2f) +
	0.0722f * std::pow(darkOutline[2], 2.2f);



bool CglFont::LoadConfigFonts()
{
	RECOIL_DETAILED_TRACY_ZONE;
	font      = CglFont::LoadFont("", false);
	smallFont = CglFont::LoadFont("", true);

	if (font      == nullptr)
		throw content_error("Failed to load FontFile \"" + configHandler->GetString("FontFile") + "\", did you forget to run make install?");

	if (smallFont == nullptr)
		throw content_error("Failed to load SmallFontFile \"" + configHandler->GetString("SmallFontFile") + "\", did you forget to run make install?");

	return true;
}

bool CglFont::LoadCustomFonts(const std::string& smallFontFile, const std::string& largeFontFile)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (auto newFont = CglFont::LoadFont(largeFontFile, false); newFont) {
		font = newFont;

		LOG("[%s] loaded large font \"%s\"", __func__, newFont->GetFilePath().c_str());
		configHandler->SetString(     "FontFile", newFont->GetFilePath());
	}

	if (auto newFont = CglFont::LoadFont(smallFontFile, false); newFont) {
		smallFont = newFont;

		LOG("[%s] loaded small font \"%s\"", __func__, newFont->GetFilePath().c_str());
		configHandler->SetString("SmallFontFile", newFont->GetFilePath());
	}

	return true;
}

std::shared_ptr<CglFont> CglFont::LoadFont(const std::string& fontFileOverride, bool smallFont)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const std::string fontFiles[] = {configHandler->GetString("FontFile"), configHandler->GetString("SmallFontFile")};
	const std::string& fontFile = (fontFileOverride.empty())? fontFiles[smallFont]: fontFileOverride;

	const   int fontSizes[] = {configHandler->GetInt("FontSize"), configHandler->GetInt("SmallFontSize")};
	const   int fontWidths[] = {configHandler->GetInt("FontOutlineWidth"), configHandler->GetInt("SmallFontOutlineWidth")};
	const float fontWeights[] = {configHandler->GetFloat("FontOutlineWeight"), configHandler->GetFloat("SmallFontOutlineWeight")};

	return CglFont::LoadFont(fontFile, fontSizes[smallFont], fontWidths[smallFont], fontWeights[smallFont]);
}


std::shared_ptr<CglFont> CglFont::LoadFont(const std::string& fontFile, int size, int outlinewidth, float outlineweight)
{
	RECOIL_DETAILED_TRACY_ZONE;
	try {
		//return (new CglFont(fontFile, size, outlinewidth, outlineweight));
		auto fnt = FindFont(fontFile, size, outlinewidth, outlineweight);
		if (fnt)
			return fnt;

		return std::static_pointer_cast<CglFont>(
			allFonts.emplace_back(
				std::make_shared<CglFont>(fontFile, size, outlinewidth, outlineweight)
			)
			.lock()
		);

	} catch (const content_error& ex) {
		LOG_L(L_ERROR, "Failed creating font: %s", ex.what());
		return nullptr;
	}
}

std::shared_ptr<CglFont> CglFont::FindFont(const std::string& fontFile, int size, int outlinewidth, float outlineweight)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const auto cmpFunc = [&fontFile, size, outlinewidth, outlineweight](std::weak_ptr<CFontTexture> item) {
		std::shared_ptr<CglFont> font = std::static_pointer_cast<CglFont>(item.lock());
		return
			size == font->GetSize() &&
			outlinewidth == font->GetOutlineWidth() &&
			outlineweight == font->GetOutlineWeight() &&
			fontFile == font->GetFilePath();
	};

	// check for unused fonts and search in the same time
	size_t fontIndex = size_t(-1);
	for (size_t i = 0; i < allFonts.size(); /*NOOP*/) {
		if (allFonts[i].expired()) {
			allFonts[i] = std::move(allFonts.back());
			allFonts.pop_back();
		}
		else {
			if (fontIndex == size_t(-1) && cmpFunc(allFonts[i]))
				fontIndex = i;

			++i;
		}
	}

	if (fontIndex == size_t(-1))
		return nullptr;

	return std::static_pointer_cast<CglFont>(allFonts[fontIndex].lock());
}


void CglFont::ReallocSystemFontAtlases(bool pre)
{
	RECOIL_DETAILED_TRACY_ZONE;
#ifdef _DEBUG
	size_t fontsCounter = 0;
	for (const auto& f : allFonts) {
		if (!f.expired())
			++fontsCounter;
	}

	assert(fontsCounter <= 2);
#endif

	if (font != nullptr)		
		font->ReallocAtlases(pre);
	if (smallFont != nullptr && smallFont != font)
		smallFont->ReallocAtlases(pre);
}

CglFont::CglFont(const std::string& fontFile, int size, int _outlineWidth, float _outlineWeight)
	: CTextWrap(fontFile, size, _outlineWidth, _outlineWeight)
	, fontPath(fontFile)
	, stringWidth {
		1 << 12,
		[f = this](const std::string& str) { return f->GetTextWidth_(toustring(str)); },
		[](const std::string& str, const auto& cache) {}, //don't save anything
	}
	, stringHeight {
		1 << 10,
		[f = this](const std::string& str) { HeightCache hc; hc.height = f->GetTextHeight_(toustring(str), &hc.descender, &hc.numLines); return hc; },
		[](const std::string& str, const auto& cache) {}, //don't save anything
	}
{
	textColor    = white;
	outlineColor = darkOutline;

	viewMatrix = DefViewMatrix();
	projMatrix = DefProjMatrix();
}

#ifdef HEADLESS
void CglFont::Begin(bool userDefinedBlending) {}
void CglFont::End() {}
void CglFont::DrawBuffered(bool userDefinedBlending) {}
void CglFont::DrawWorldBuffered(bool userDefinedBlending) {}

void CglFont::glWorldPrint(const float3& p, const float size, const std::string& str, int options) {}

CMatrix44f CglFont::DefViewMatrix() { return CMatrix44f::Identity(); }
CMatrix44f CglFont::DefProjMatrix() { return CMatrix44f::Identity(); }

void CglFont::glPrint(float x, float y, float s, const int options, const std::string& str) {}
void CglFont::glPrintTable(float x, float y, float s, const int options, const std::string& str) {}

void CglFont::SetAutoOutlineColor(bool enable) {}
void CglFont::SetTextColor(const SColor* color) {}
void CglFont::SetOutlineColor(const SColor* color) {}
void CglFont::SetColors(const SColor* textColor, const SColor* outlineColor) {}

float CglFont::GetCharacterWidth(const char32_t c) { return 1.0f; }
void CglFont::ScanForWantedGlyphs(const spring::u8string& str) {}
float CglFont::GetTextWidth_(const spring::u8string& text) { return (text.size() * 1.0f); }
float CglFont::GetTextHeight_(const spring::u8string& text, float* descender, int* numLines) { return 1.0f; }

std::deque<std::string> CglFont::SplitIntoLines(const spring::u8string& text) { return {}; }

void CglFont::GetStats(std::array<size_t, 8>& stats) const {}

#else

class WidthCalculator : public TextIteratorHandler {
private:
	CglFont* font;
	float curw = 0.0f;
	float maxw = 0.0f;
	char32_t prvGlyphIdx = 0;
	const GlyphInfo* prvGlyphPtr = nullptr;

public:
	explicit WidthCalculator(CglFont* f) : font(f) {}

	bool OnNewline(const CharEvent& e) override {
		if (prvGlyphPtr != nullptr)
			curw += font->GetCharacterWidth(prvGlyphIdx);

		maxw = std::max(curw, maxw);
		curw = 0.0f;
		prvGlyphPtr = nullptr;
		prvGlyphIdx = 0;
		return true;
	}

	bool OnSpace(const CharEvent& e) override {
		return OnPrintable(e); // Treat space as printable
	}

	bool OnPrintable(const CharEvent& e) override {
		const GlyphInfo* curGlyphPtr = &font->GetGlyph(std::get<char32_t>(e.value));
		assert(curGlyphPtr != &CFontTexture::dummyGlyph);

		if (prvGlyphPtr != nullptr) {
			prvGlyphPtr = &font->GetGlyph(prvGlyphIdx);
			assert(prvGlyphPtr != &CFontTexture::dummyGlyph);
			curw += font->GetKerning(*prvGlyphPtr, *curGlyphPtr);
		}

		prvGlyphPtr = curGlyphPtr;
		prvGlyphIdx = std::get<char32_t>(e.value);
		return true;
	}

	void OnEnd() override {
		if (prvGlyphPtr != nullptr)
			curw += font->GetCharacterWidth(prvGlyphIdx);
	}
public:
	float getWidth() const { return std::max(curw, maxw); }
};

float CglFont::GetTextWidth_(const spring::u8string& text)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (text.empty())
		return 0.0f;

	auto lock = sync.GetScopedLock();
	ScanForWantedGlyphs(text);

	WidthCalculator calc(this);
	TextIterator iterator(text, calc);
	iterator.Execute();

	return calc.getWidth();
}

float CglFont::GetCharacterWidth(const char32_t c)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const auto& glyph = GetGlyph(c);
	assert(&glyph != &CFontTexture::dummyGlyph);
	return glyph.advance;
}

class HeightCalculator : public TextIteratorHandler {
private:
	CglFont* font;
	float h = 0.0f;
	float d;
	unsigned int multiLine = 1;

public:
	explicit HeightCalculator(CglFont* f)
		: font(f)
		, d(f->GetLineHeight() + f->GetDescender())
	{}

	bool OnNewline(const CharEvent& e) {
		multiLine++;
		d = font->GetLineHeight() + font->GetDescender();
		return true;
	}

	bool OnPrintable(const CharEvent& e) {
		const GlyphInfo& g = font->GetGlyph(std::get<char32_t>(e.value));
		assert(&g != &CFontTexture::dummyGlyph);

		d = std::min(d, g.descender);
		h = std::max(h, g.height * (multiLine < 2));
		return true;
	}

	bool OnSpace(const CharEvent& e) {
		return OnPrintable(e); // Treat space as printable
	}

	void OnEnd() {
		d -= ((multiLine - 1) * font->GetLineHeight() * (multiLine > 1));
	}
public:
	float getHeight() const { return h; }
	float getDescender() const { return d; }
	int getNumLines() const { return multiLine; }
};


float CglFont::GetTextHeight_(const spring::u8string& text, float* descender, int* numLines)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (text.empty()) {
		if (descender != nullptr) *descender = 0.0f;
		if (numLines != nullptr) *numLines = 0;
		return 0.0f;
	}

	auto lock = sync.GetScopedLock();
	ScanForWantedGlyphs(text);

	HeightCalculator calc(this);
	TextIterator iterator(text, calc);
	iterator.Execute();

	if (descender != nullptr) *descender = calc.getDescender();
	if (numLines != nullptr) *numLines = calc.getNumLines();

	return calc.getHeight();
}

class GlyphScanner : public TextIteratorHandler {
private:
	CglFont* font;
	std::vector<char32_t>& missingGlyphs;

public:
	GlyphScanner(CglFont* f, std::vector<char32_t>& missing)
		: font(f)
		, missingGlyphs(missing)
	{}

	bool OnPrintable(const CharEvent& e) {
		const GlyphInfo& curGlyph = font->GetGlyph(std::get<char32_t>(e.value));
		if (&curGlyph == &CFontTexture::dummyGlyph)
			missingGlyphs.emplace_back(std::get<char32_t>(e.value));
		return true;
	}
};

void CglFont::ScanForWantedGlyphs(const spring::u8string& ustr)
{
	RECOIL_DETAILED_TRACY_ZONE;
	static std::vector<char32_t> missingGlyphs;
	missingGlyphs.clear();

	GlyphScanner scanner(this, missingGlyphs);
	TextIterator iterator(ustr, scanner);
	iterator.Execute();

	LoadWantedGlyphs(missingGlyphs);
}

struct SplitIntoLinesHandler final : TextIteratorHandler {
	const spring::u8string& text;

	std::deque<std::string> lines;
	std::deque<std::string> colorCodeStack;

	explicit SplitIntoLinesHandler(const spring::u8string& text)
		: text(text)
	{
		lines.emplace_back("");
	}

	void AppendBytesToCurrentLine(const CharEvent& e)
	{
		auto& out = lines.back(); // last element [web:18]
		for (int i = e.startIdx; i < e.endIdx; ++i) {
			out.push_back(static_cast<char>(text[i]));
		}
	}

	void StartNewLineWithActiveColor()
	{
		lines.emplace_back("");

		// Old code used only top-of-stack (#else path).
		if (!colorCodeStack.empty())
			lines.back() = colorCodeStack.back(); // back() is O(1) [web:18]
	}

	bool OnColorCode(const CharEvent& e) override
	{
		// Only keep it if there is more text after the full code sequence (old: idx+4 < end).
		if (e.endIdx < static_cast<int>(text.size())) {
			auto cc = e.ToString();
			colorCodeStack.emplace_back(cc);
			lines.back() += cc;
		}
		return true;
	}

	bool OnColorCodeEx(const CharEvent& e) override
	{
		// Only keep it if there is more text after the full code sequence (old: idx+9 < end).
		if (e.endIdx < static_cast<int>(text.size())) {
			auto cc = e.ToString();
			colorCodeStack.emplace_back(cc);
			lines.back() += cc;
		}
		return true;
	}

	bool OnColorReset(const CharEvent& e) override
	{
		if (!colorCodeStack.empty())
			colorCodeStack.pop_back(); // removes last element [web:16]
		AppendBytesToCurrentLine(e);   // keep the reset marker in the output like the old code did
		return true;
	}

	bool OnNewline(const CharEvent&) override
	{
		StartNewLineWithActiveColor();
		return true;
	}

	bool OnPrintable(const CharEvent& e) override
	{
		AppendBytesToCurrentLine(e);
		return true;
	}

	bool OnSpace(const CharEvent& e) override
	{
		AppendBytesToCurrentLine(e);
		return true;
	}
};


std::deque<std::string> CglFont::SplitIntoLines(const spring::u8string& text)
{
	RECOIL_DETAILED_TRACY_ZONE;

	if (text.empty())
		return {};

	SplitIntoLinesHandler h(text);
	TextIterator it(text, h);
	it.Execute();

	return std::move(h.lines);
}

void CglFont::SetAutoOutlineColor(bool enable)
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto lock = sync.GetScopedLock();

	autoOutlineColor = enable;
}

void CglFont::SetTextColor(const SColor* color)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (color == nullptr)
		color = &white;

	auto lock = sync.GetScopedLock();

	textColor = *color;
}

void CglFont::SetOutlineColor(const SColor* color)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (color == nullptr)
		color = ChooseOutlineColor(textColor);

	auto lock = sync.GetScopedLock();

	outlineColor = *color;
}


void CglFont::SetColors(const SColor* _textColor, const SColor* _outlineColor)
{
	RECOIL_DETAILED_TRACY_ZONE;
	SetTextColor(_textColor);
	SetOutlineColor(_outlineColor);
}

const SColor* CglFont::ChooseOutlineColor(const SColor& textColor)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const auto fTextColor = static_cast<float4>(textColor);
	const float luminosity =
		0.2126f * std::pow(fTextColor[0], 2.2f) +
		0.7152f * std::pow(fTextColor[1], 2.2f) +
		0.0722f * std::pow(fTextColor[2], 2.2f);

	const float maxLum = std::max(luminosity + 0.05f, darkLuminosity);
	const float minLum = std::min(luminosity + 0.05f, darkLuminosity);

	if ((maxLum / minLum) > 5.0f)
		return &darkOutline;

	return &lightOutline;
}

void CglFont::Begin(bool userDefinedBlending) {
	RECOIL_DETAILED_TRACY_ZONE;
	sync.Lock();

	fontRenderer->SetUserDefinedBlending(userDefinedBlending);

	if (inBeginEndBlock) {
		sync.Unlock();
		return;
	}

	inBeginEndBlock = true;
}

void CglFont::End() {
	RECOIL_DETAILED_TRACY_ZONE;
	if (!inBeginEndBlock) {
		LOG_L(L_ERROR, "called End() without Begin()");
		return;
	}
	inBeginEndBlock = false;

	//without this, fonts textures are empty in display lists (probably GL commands in UploadGlyphAtlasTexture are get recorded as part of the list)
	fontRenderer->HandleTextureUpdate(*this, false);
	fontRenderer->PushGLState(*this);
	fontRenderer->DrawTraingleElements();
	fontRenderer->PopGLState(*this);

	fontRenderer->SetUserDefinedBlending(false);

	inBeginEndBlock = false;
	sync.Unlock();
}


void CglFont::DrawBuffered(bool userDefinedBlending)
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto lock = sync.GetScopedLock();

	UpdateGlyphAtlasTexture();
	UploadGlyphAtlasTexture();

	fontRenderer->SetUserDefinedBlending(userDefinedBlending);

	fontRenderer->PushGLState(*this);
	fontRenderer->DrawTraingleElements();
	fontRenderer->PopGLState(*this);

	fontRenderer->SetUserDefinedBlending(false);
}

void CglFont::DrawWorldBuffered(bool userDefinedBlending)
{
	RECOIL_DETAILED_TRACY_ZONE;
	glPushMatrix();
	glMultMatrixf(camera->GetBillBoardMatrix());

	DrawBuffered(userDefinedBlending);

	glPopMatrix();
}

template<int shiftXC, int shiftYC, bool outline>
class StringRenderer : public TextIteratorHandler {
private:
	CglFont* font;
	float x, y, startx, scaleX, scaleY, lineHeight_;
	char32_t prvGlyphIdx = 0;
	int skippedLines = 0;

public:
	StringRenderer(CglFont* f, float x, float y, float sX, float sY)
		: font(f), x(x), y(y), startx(x), scaleX(sX), scaleY(sY)
		, lineHeight_(sY* f->GetLineHeight()) {
	}

	bool OnColorCode(const CharEvent& e) override {
		const auto& newTextColor = std::get<SColor>(e.value);

		if (font->autoOutlineColor)
			font->SetColors(&newTextColor, nullptr);
		else
			font->SetTextColor(&newTextColor);

		return true;
	}

	bool OnColorCodeEx(const CharEvent& e) override {
		const auto& colors = std::get<FontColors>(e.value);
		font->SetColors(&colors.textColor, &colors.outlineColor);
		return true;
	}

	bool OnColorReset(const CharEvent& e) override {
		font->SetColors(&font->baseTextColor, &font->baseOutlineColor);
		return true;
	}

	bool OnNewline(const CharEvent& e) override {
		x = startx;
		y -= (e.linesSkipped * lineHeight_);
		prvGlyphIdx = 0;
		return true;
	}

	bool OnSpace(const CharEvent& e) override {
		return OnPrintable(e);
	}

	bool OnPrintable(const CharEvent& e) override {
		const GlyphInfo* curGlyphPtr = &font->GetGlyph(std::get<char32_t>(e.value));
		assert(curGlyphPtr != &CFontTexture::dummyGlyph);

		if (prvGlyphIdx != 0) {
			const GlyphInfo* prvGlyphPtr = &font->GetGlyph(prvGlyphIdx);
			assert(prvGlyphPtr != &CFontTexture::dummyGlyph);
			x += (scaleX * font->GetKerning(*prvGlyphPtr, *curGlyphPtr));
		}

		prvGlyphIdx = std::get<char32_t>(e.value);

		const auto& tc = curGlyphPtr->texCord;
		const float dx0 = (scaleX * curGlyphPtr->size.x0()) + x;
		const float dy0 = (scaleY * curGlyphPtr->size.y0()) + y;
		const float dx1 = (scaleX * curGlyphPtr->size.x1()) + x;
		const float dy1 = (scaleY * curGlyphPtr->size.y1()) + y;
		const float tx0 = tc.x0();
		const float ty0 = tc.y0();
		const float tx1 = tc.x1();
		const float ty1 = tc.y1();

		const auto& textDepth = font->textDepth;
		const auto& textColor = font->textColor;
		const auto& outlineColor = font->outlineColor;
		const auto fontSize = font->fontSize;

		auto& fontRenderer = font->fontRenderer;

		if constexpr (shiftXC > 0 || shiftYC > 0 || outline) {
			const auto& stc = curGlyphPtr->shadowTexCord;
			float shiftX = (shiftXC > 0) ? scaleX * shiftXC / 100.0f : 0.0f;
			float shiftY = (shiftYC > 0) ? scaleY * shiftYC / 100.0f : 0.0f;
			float ssX = outline ? (scaleX / fontSize) * font->GetOutlineWidth() : 0.0f;
			float ssY = outline ? (scaleY / fontSize) * font->GetOutlineWidth() : 0.0f;

			fontRenderer->AddQuadTrianglesOB(
				{ {dx0 + shiftX - ssX, dy0 - shiftY + ssY, textDepth.y}, stc.x0(), stc.y0(), outlineColor },
				{ {dx1 + shiftX + ssX, dy0 - shiftY + ssY, textDepth.y}, stc.x1(), stc.y0(), outlineColor },
				{ {dx1 + shiftX + ssX, dy1 - shiftY - ssY, textDepth.y}, stc.x1(), stc.y1(), outlineColor },
				{ {dx0 + shiftX - ssX, dy1 - shiftY - ssY, textDepth.y}, stc.x0(), stc.y1(), outlineColor }
			);
		}

		fontRenderer->AddQuadTrianglesPB(
			{ {dx0, dy0, textDepth.x}, tx0, ty0, textColor },
			{ {dx1, dy0, textDepth.x}, tx1, ty0, textColor },
			{ {dx1, dy1, textDepth.x}, tx1, ty1, textColor },
			{ {dx0, dy1, textDepth.x}, tx0, ty1, textColor }
		);

		return true;
	}
};

template<int shiftXC, int shiftYC, bool outline>
void CglFont::RenderStringImpl(float x, float y, float scaleX, float scaleY, const std::string& str)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const spring::u8string& ustr = toustring(str);
	ScanForWantedGlyphs(ustr);

	StringRenderer<shiftXC, shiftYC, outline> renderer(this, x, y, scaleX, scaleY);
	TextIterator iterator(ustr, renderer);
	iterator.Execute();
}

void CglFont::glWorldPrint(const float3& p, const float size, const std::string& str, int options)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const bool buffered = (options & FONT_BUFFERED) == FONT_BUFFERED;
	if (!buffered) {
		glPushMatrix();

		CMatrix44f tbM = camera->GetBillBoardMatrix();
		//tbM.SetPos(p); // (Tr * Bm)
		glMultMatrixf(tbM);

		const float3 pos = tbM.Transpose() * p;

		Begin();
		SetTextDepth(pos.z); SetOutlineDepth(pos.z);
		glPrint(pos.x, pos.y, size, options, str);
		SetTextDepth(     ); SetOutlineDepth(     );
		End();

		glPopMatrix();
	}
	else {
		CMatrix44f bm = camera->GetBillBoardMatrix();
		const float3 drawPos = bm.Transpose() * p;

		// drawPos negates the effect of multiplication by camera->GetBillBoardMatrix() in DrawWorldBuffered

		SetTextDepth(drawPos.z); SetOutlineDepth(drawPos.z);
		glPrint(drawPos.x, drawPos.y, size, options | FONT_BUFFERED, str);
		SetTextDepth(         ); SetOutlineDepth(         );
	}
}

CMatrix44f CglFont::DefViewMatrix() { return CMatrix44f::Identity(); }
CMatrix44f CglFont::DefProjMatrix() { return CMatrix44f::ClipOrthoProj01(); }



void CglFont::glPrint(float x, float y, float s, const int options, const std::string& text)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// s := scale or absolute size?
	if (options & FONT_SCALE)
		s *= fontSize;

	float sizeX = s;
	float sizeY = s;
	float textDescender = 0.0f;

	// render in normalized coords (0..1) instead of screencoords (0..~1024)
	if (options & FONT_NORM) {
		sizeX *= globalRendering->pixelX;
		sizeY *= globalRendering->pixelY;
	}

	// horizontal alignment (FONT_LEFT is default)
	if (options & FONT_CENTER) {
		x -= (sizeX * 0.5f * GetTextWidth(text));
	} else if (options & FONT_RIGHT) {
		x -= (sizeX * GetTextWidth(text));
	}


	// vertical alignment
	y += (sizeY * GetDescender()); // move to baseline (note: descender is negative)

	if (options & FONT_BASELINE) {
		// nothing
	} else if (options & FONT_DESCENDER) {
		y -= (sizeY * GetDescender());
	} else if (options & FONT_VCENTER) {
		y -= (sizeY * 0.5f * GetTextHeight(text, &textDescender));
		y -= (sizeY * 0.5f * textDescender);
	} else if (options & FONT_TOP) {
		y -= sizeY * GetTextHeight(text);
	} else if (options & FONT_ASCENDER) {
		y -= (sizeY * (GetDescender() + 1.0f));
	} else if (options & FONT_BOTTOM) {
		GetTextHeight(text, &textDescender);
		y -= (sizeY * textDescender);
	}

	if (options & FONT_NEAREST) {
		x = (int)x;
		y = (int)y;
	}

	// backup text & outline colors, ::ColorResetIndicator will reset them
	baseTextColor = textColor;
	baseOutlineColor = outlineColor;

	const bool buffered = ((options & FONT_BUFFERED) != 0);
	const bool immediate = (!inBeginEndBlock && !buffered);

	if (immediate) {
		// no buffering
		Begin();
	} else if (buffered) {
		if (!inBeginEndBlock)
			sync.Lock();
	}

	// select correct decoration RenderString function
	if ((options & FONT_OUTLINE) != 0) {
		RenderStringOutlined(x, y, sizeX, sizeY, text);
	} else if ((options & FONT_SHADOW) != 0) {
		RenderStringShadow(x, y, sizeX, sizeY, text);
	} else {
		RenderString(x, y, sizeX, sizeY, text);
	}

	if (immediate) {
		End();
	} else if (buffered) {
		if (!inBeginEndBlock)
			sync.Unlock();
	}

	// reset text & outline colors (if changed via in-text colorcodes)
	SetColors(&baseTextColor, &baseOutlineColor);
}

// leave unmodified for now
#if 0
class TablePrinter : public TextIteratorHandler {
private:
    const spring::u8string& text;
    std::vector<std::string>& colLines;
    std::vector<SColor>& colColor;
    SColor& curColor;
    const SColor& defColor;
    int& col;
    int& row;
    
public:
    TablePrinter(
        const spring::u8string& text,
        std::vector<std::string>& colLines,
        std::vector<SColor>& colColor,
        SColor& curColor,
        const SColor& defColor,
        int& col,
        int& row
    )
        : text(text)
        , colLines(colLines)
        , colColor(colColor)
        , curColor(curColor)
        , defColor(defColor)
        , col(col)
        , row(row)
    {}
    
    bool OnColorCode(const CharEvent& e) override {
        // Add color code bytes to current column
        for (int i = e.startIdx; i < e.endIdx && i < text.length(); ++i) {
            colLines[col] += text[i];
        }
        
		curColor = std::get<SColor>(e.value);
        return true;
    }

    bool OnColorReset(const CharEvent& e) override {
        colLines[col] += ColorResetIndicator;
        return true;
    }
    
    bool OnNewline(const CharEvent& e) override {
        for (auto& colLine : colLines)
            colLine += LF;
        
        // Reset color in first column if needed
        if (colColor[0] != curColor) {
            for (int i = 0; i < 4; ++i)
                colLines[0] += curColor[i];
            colColor[0] = curColor;
        }
        
        col = 0;
        row += 1;
        return true;
    }
    
    bool OnSpace(const CharEvent& e) override {
        if (std::get<char32_t>(e.value) == '\t') {
            // Tab is column separator
            if ((col += 1) >= colLines.size()) {
                colLines.emplace_back("");
                // Add newlines for current row
                for (int i = 0; i < row; ++i)
                    colLines[col] += LF;
                colColor.push_back(defColor);
            }
            // Apply current color to new column if different
            if (colColor[col] != curColor) {
                for (int i = 0; i < 4; ++i)
                    colLines[col] += curColor[i];
                colColor[col] = curColor;
            }
        } else {
            // Regular space - add original bytes
            for (int i = e.startIdx; i < e.endIdx; ++i) {
                colLines[col] += text[i];
            }
        }
        return true;
    }
    
    bool OnPrintable(const CharEvent& e) override {
        // Add original character bytes to current column
        for (int i = e.startIdx; i < e.endIdx; ++i) {
            colLines[col] += text[i];
        }
        return true;
    }
};

void CglFont::glPrintTable(float x, float y, float s, const int options, const std::string& text)
{
	RECOIL_DETAILED_TRACY_ZONE;

	const spring::u8string& ustr = toustring(text);

	std::vector<std::string> colLines;
	std::vector<float> colWidths;
	std::vector<SColor> colColor;

	SColor defColor(255, 0, 0);
	SColor curColor(255, 0, 0);

	for (int i = 0; i < 3; ++i) {
		defColor[i + 1] = uint8_t(textColor[i] * 255.0f);
		curColor[i + 1] = defColor[i + 1];
	}

	colLines.emplace_back("");
	colColor.push_back(defColor);

	int col = 0;
	int row = 0;

	// Parse text into columns using TextIterator
	TablePrinter printer(ustr, colLines, colColor, curColor, defColor, col, row);
	TextIterator iterator(ustr, printer);
	iterator.Execute();

	// Calculate dimensions
	float totalWidth = 0.0f;
	float maxHeight = 0.0f;
	float minDescender = 0.0f;

	colWidths.resize(colLines.size(), 0.0f);

	for (size_t i = 0; i < colLines.size(); ++i) {
		colWidths[i] = GetTextWidth(colLines[i]);
		totalWidth += colWidths[i];

		float textDescender;
		float textHeight = GetTextHeight(colLines[i], &textDescender);

		maxHeight = std::max(maxHeight, textHeight);
		minDescender = std::min(minDescender, textDescender);
	}

	// Calculate size
	float ss = s;
	if (options & FONT_SCALE)
		ss *= fontSize;

	float sizeX = ss;
	float sizeY = ss;

	// Render in normalized coords (0..1) instead of screencoords (0..~1024)
	if (options & FONT_NORM) {
		sizeX *= globalRendering->pixelX;
		sizeY *= globalRendering->pixelY;
	}

	// Horizontal alignment (FONT_LEFT is default)
	if (options & FONT_CENTER) {
		x -= (sizeX * 0.5f * totalWidth);
	}
	else if (options & FONT_RIGHT) {
		x -= (sizeX * totalWidth);
	}

	// Vertical alignment
	if (options & FONT_BASELINE) {
		// nothing
	}
	else if (options & FONT_DESCENDER) {
		y -= (sizeY * GetDescender());
	}
	else if (options & FONT_VCENTER) {
		y -= (sizeY * 0.5f * maxHeight);
		y -= (sizeY * 0.5f * minDescender);
	}
	else if (options & FONT_TOP) {
		y -= (sizeY * maxHeight);
	}
	else if (options & FONT_ASCENDER) {
		y -= (sizeY * (GetDescender() + 1.0f));
	}
	else if (options & FONT_BOTTOM) {
		y -= (sizeY * minDescender);
	}

	// Render each column
	for (size_t i = 0; i < colLines.size(); ++i) {
		glPrint(x, y, s, (options | FONT_BASELINE) & ~(FONT_RIGHT | FONT_CENTER), colLines[i]);
		x += (sizeX * colWidths[i]);
	}
}

#else
// TODO: remove, only used by PlayerRosterDrawer
void CglFont::glPrintTable(float x, float y, float s, const int options, const std::string& text)
{
	RECOIL_DETAILED_TRACY_ZONE;
	std::vector<std::string> colLines;
	std::vector<float> colWidths;
	std::vector<SColor> colColor;

	SColor defColor(255, 0, 0);
	SColor curColor(255, 0, 0);

	for (int i = 0; i < 3; ++i) {
		defColor[i + 1] = uint8_t(textColor[i] * 255.0f);
		curColor[i + 1] = defColor[i + 1];
	}

	colLines.emplace_back("");
	colColor.push_back(defColor);

	int col = 0;
	int row = 0;

	for (int pos = 0; pos < text.length(); pos++) {
		const unsigned char& c = text[pos];

		switch (c) {
			case OldColorCodeIndicator: {
				if (fontHandler.disableOldColorIndicators) {
					// same as 'default' case
					colLines[col] += c;
					break;
				}
				[[fallthrough]];
			}
			// inline colorcodes
			case ColorCodeIndicator: {
				for (int i = 0; i < 4 && pos < text.length(); ++i, ++pos) {
					colLines[col] += text[pos];
					curColor[i] = text[pos];
				}
				colColor[col] = curColor;
				pos -= 1;
			} break;
			case OldColorCodeIndicatorEx: {
				if (fontHandler.disableOldColorIndicators) {
					// same as 'default' case
					colLines[col] += c;
					break;
				}
				[[fallthrough]];
			}
			case ColorCodeIndicatorEx: {
				assert(false); //not implemented
			} break;

			// column separator is horizontal tab
			case '\t': {
				if ((col += 1) >= colLines.size()) {
					colLines.emplace_back("");
					for (int i = 0; i < row; ++i)
						colLines[col] += LF;
					colColor.push_back(defColor);
				}
				if (colColor[col] != curColor) {
					for (int i = 0; i < 4; ++i)
						colLines[col] += curColor[i];
					colColor[col] = curColor;
				}
			} break;

			case CR: {
				pos += ((pos + 1) < text.length() && text[pos + 1] == LF);
				[[fallthrough]]; // CR; fall-through
			}
			case LF: {
				// LF
				for (auto& colLine: colLines)
					colLine += LF;

				if (colColor[0] != curColor) {
					for (int i = 0; i < 4; ++i)
						colLines[0] += curColor[i];
					colColor[0] = curColor;
				}

				col = 0;
				row += 1;
			} break;

			// printable char or orphaned (c >= 127 && c <= 255) color-code
			default: {
				colLines[col] += c;
			} break;
		}
	}

	float totalWidth = 0.0f;
	float maxHeight = 0.0f;
	float minDescender = 0.0f;

	colWidths.resize(colLines.size(), 0.0f);

	for (size_t i = 0; i < colLines.size(); ++i) {
		colWidths[i] = GetTextWidth(colLines[i]);
		totalWidth += colWidths[i];

		float textDescender;
		float textHeight = GetTextHeight(colLines[i], &textDescender);

		maxHeight = std::max(maxHeight, textHeight);
		minDescender = std::min(minDescender, textDescender);
	}

	// s := scale or absolute size?
	float ss = s;
	if (options & FONT_SCALE)
		ss *= fontSize;

	float sizeX = ss;
	float sizeY = ss;

	// render in normalized coords (0..1) instead of screencoords (0..~1024)
	if (options & FONT_NORM) {
		sizeX *= globalRendering->pixelX;
		sizeY *= globalRendering->pixelY;
	}

	// horizontal alignment (FONT_LEFT is default)
	if (options & FONT_CENTER) {
		x -= (sizeX * 0.5f * totalWidth);
	} else if (options & FONT_RIGHT) {
		x -= (sizeX * totalWidth);
	}

	// vertical alignment
	if (options & FONT_BASELINE) {
		// nothing
	} else if (options & FONT_DESCENDER) {
		y -= (sizeY * GetDescender());
	} else if (options & FONT_VCENTER) {
		y -= (sizeY * 0.5f * maxHeight);
		y -= (sizeY * 0.5f * minDescender);
	} else if (options & FONT_TOP) {
		y -= (sizeY * maxHeight);
	} else if (options & FONT_ASCENDER) {
		y -= (sizeY * (GetDescender() + 1.0f));
	} else if (options & FONT_BOTTOM) {
		y -= (sizeY * minDescender);
	}

	for (size_t i = 0; i < colLines.size(); ++i) {
		glPrint(x, y, s, (options | FONT_BASELINE) & ~(FONT_RIGHT | FONT_CENTER), colLines[i]);
		x += (sizeX * colWidths[i]);
	}
}
#endif

void CglFont::GetStats(std::array<size_t, 8>& stats) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	fontRenderer->GetStats(stats);
}

#endif

