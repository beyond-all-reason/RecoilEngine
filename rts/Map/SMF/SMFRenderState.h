/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SMF_RENDERSTATE_H
#define SMF_RENDERSTATE_H

#include <array>
#include "Map/MapDrawPassTypes.h"

class CSMFGroundDrawer;
struct ISkyLight;
struct LuaMapShaderData;

namespace Shader {
	struct IProgramObject;
}

enum {
	RENDER_STATE_FFP = 0, // fixed-function path
	RENDER_STATE_SSP = 1, // standard-shader path (ARB/GLSL)
	RENDER_STATE_LUA = 2, // Lua-shader path
	RENDER_STATE_NOP = 3, // NO-OP path
	RENDER_STATE_SEL = 4, // selected path
	RENDER_STATE_CNT = 5,
};


struct ISMFRenderState {
public:
	static ISMFRenderState* GetInstance(bool haveGLSL, bool luaShader, bool noop);
	static void FreeInstance(ISMFRenderState* state) { delete state; }

	virtual ~ISMFRenderState() {}
	virtual bool Init(const CSMFGroundDrawer* smfGroundDrawer) = 0;
	virtual void Kill() = 0;
	virtual void Update(
		const CSMFGroundDrawer* smfGroundDrawer,
		const LuaMapShaderData* luaMapShaderData
	) = 0;

	virtual bool HasValidShader(const DrawPass::e& drawPass) const = 0;
	virtual bool CanEnable(const CSMFGroundDrawer* smfGroundDrawer) const = 0;
	virtual bool CanDrawForward() const = 0;
	virtual bool CanDrawDeferred() const = 0;

	virtual void Enable(const CSMFGroundDrawer* smfGroundDrawer, const DrawPass::e& drawPass) = 0;
	virtual void Disable(const CSMFGroundDrawer* smfGroundDrawer, const DrawPass::e& drawPass) = 0;

	virtual void SetSquareTexGen(const int sqx, const int sqy) const = 0;
	virtual void SetCurrentShader(const DrawPass::e& drawPass) = 0;
	virtual void UpdateCurrentShaderSky(const ISkyLight* skyLight) const = 0;
};


struct SMFRenderStateNOOP : public ISMFRenderState {
public:
	~SMFRenderStateNOOP() override = default;
	bool Init(const CSMFGroundDrawer* smfGroundDrawer) override { return false; }
	void Kill() override {}
	void Update(
		const CSMFGroundDrawer* smfGroundDrawer,
		const LuaMapShaderData* luaMapShaderData
	) override {}

	bool HasValidShader(const DrawPass::e& drawPass) const override { return true; }
	bool CanEnable(const CSMFGroundDrawer* smfGroundDrawer) const override { return true; }
	bool CanDrawForward() const override { return false; }
	bool CanDrawDeferred() const override { return false; }

	void Enable(const CSMFGroundDrawer* smfGroundDrawer, const DrawPass::e& drawPass) override {}
	void Disable(const CSMFGroundDrawer* smfGroundDrawer, const DrawPass::e& drawPass) override {}

	void SetSquareTexGen(const int sqx, const int sqy) const override {}
	void SetCurrentShader(const DrawPass::e& drawPass) override {}
	void UpdateCurrentShaderSky(const ISkyLight* skyLight) const override {}
};


struct SMFRenderStateFFP: public ISMFRenderState {
public:
	bool Init(const CSMFGroundDrawer* smfGroundDrawer) override { return false; }
	void Kill() override {}
	void Update(
		const CSMFGroundDrawer* smfGroundDrawer,
		const LuaMapShaderData* luaMapShaderData
	) override {}

	bool HasValidShader(const DrawPass::e& drawPass) const override { return false; }
	bool CanEnable(const CSMFGroundDrawer* smfGroundDrawer) const override;
	bool CanDrawForward() const override { return true; }
	bool CanDrawDeferred() const override { return false; }

	void Enable(const CSMFGroundDrawer* smfGroundDrawer, const DrawPass::e& drawPass) override;
	void Disable(const CSMFGroundDrawer* smfGroundDrawer, const DrawPass::e& drawPass) override;

	void SetSquareTexGen(const int sqx, const int sqy) const override;
	void SetCurrentShader(const DrawPass::e& drawPass) override {}
	void UpdateCurrentShaderSky(const ISkyLight* skyLight) const override {};
};

struct SMFRenderStateGLSL: public ISMFRenderState {
public:
	explicit SMFRenderStateGLSL(bool lua): useLuaShaders(lua) { glslShaders.fill(nullptr); }
	~SMFRenderStateGLSL() override { glslShaders.fill(nullptr); }

	bool Init(const CSMFGroundDrawer* smfGroundDrawer) override;
	void Kill() override;
	void Update(
		const CSMFGroundDrawer* smfGroundDrawer,
		const LuaMapShaderData* luaMapShaderData
	) override;

	bool HasValidShader(const DrawPass::e& drawPass) const override;
	bool CanEnable(const CSMFGroundDrawer* smfGroundDrawer) const override;
	bool CanDrawForward() const override { return true; }
	bool CanDrawDeferred() const override { return true; }

	void Enable(const CSMFGroundDrawer* smfGroundDrawer, const DrawPass::e& drawPass) override;
	void Disable(const CSMFGroundDrawer* smfGroundDrawer, const DrawPass::e& drawPass) override;

	void SetSquareTexGen(const int sqx, const int sqy) const override;
	void SetCurrentShader(const DrawPass::e& drawPass) override;
	void UpdateCurrentShaderSky(const ISkyLight* skyLight) const override;

	enum {
		GLSL_SHADER_STANDARD = 0,
		GLSL_SHADER_DEFERRED = 1,
		GLSL_SHADER_CURRENT  = 2,
		GLSL_SHADER_COUNT    = 3,
	};

private:
	// [0] := standard version
	// [1] := deferred version (not used unless AllowDeferredMapRendering)
	// [2] := currently active GLSL shader {0, 1}
	std::array<Shader::IProgramObject*, GLSL_SHADER_COUNT> glslShaders;

	// if true, shader programs for this state are Lua-defined
	bool useLuaShaders;
};

#endif

