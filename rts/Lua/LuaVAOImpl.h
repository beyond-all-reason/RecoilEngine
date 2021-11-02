/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_VAO_IMPL_H
#define LUA_VAO_IMPL_H

#include <map>
#include <string>
#include <memory>

#include "lib/sol2/forward.hpp"
#include "Rendering/GL/myGL.h"
#include "Rendering/Models/3DModelVAO.h"

class VAO;
class VBO;
class LuaVBOImpl;

class LuaVAOImpl {
public:
	LuaVAOImpl();

	LuaVAOImpl(const LuaVAOImpl& lva) = delete;
	LuaVAOImpl(LuaVAOImpl&& lva) = default;

	void Delete();
	~LuaVAOImpl();
public:
	static bool Supported();
public:
	using LuaVBOImplSP = std::shared_ptr<LuaVBOImpl>; //my workaround to https://github.com/ThePhD/sol2/issues/1206
public:
	void AttachVertexBuffer(const LuaVBOImplSP& luaVBO);
	void AttachInstanceBuffer(const LuaVBOImplSP& luaVBO);
	void AttachIndexBuffer(const LuaVBOImplSP& luaVBO);

	void DrawArrays(GLenum mode, sol::optional<GLsizei> vertCountOpt, sol::optional<GLint> vertexFirstOpt, sol::optional<int> instanceCountOpt, sol::optional<int> instanceFirstOpt);
	void DrawElements(GLenum mode, sol::optional<GLsizei> indCountOpt, sol::optional<int> indElemOffsetOpt, sol::optional<int> instanceCountOpt, sol::optional<int> baseVertexOpt);

	void ClearSubmission();
	int AddUnitsToSubmission(int id);
	int AddUnitsToSubmission(const sol::stack_table& ids);
	int AddFeaturesToSubmission(int id);
	int AddFeaturesToSubmission(const sol::stack_table& ids);
	int AddUnitDefsToSubmission(int id);
	int AddUnitDefsToSubmission(const sol::stack_table& ids);
	int AddFeatureDefsToSubmission(int id);
	int AddFeatureDefsToSubmission(const sol::stack_table& ids);

	void RemoveFromSubmission(int idx);

	void Submit();
private:
	std::pair<GLsizei, GLsizei> DrawCheck(GLenum mode, sol::optional<GLsizei> drawCountOpt, sol::optional<int> instanceCountOpt, bool indexed);
	void CondInitVAO();
	void CheckDrawPrimitiveType(GLenum mode) const;
	void AttachBufferImpl(const std::shared_ptr<LuaVBOImpl>& luaVBO, std::shared_ptr<LuaVBOImpl>& thisLuaVBO, GLenum reqTarget);
private:
	template <typename TObj>
	int AddObjectsToSubmissionImpl(int id);
	template <typename TObj>
	int AddObjectsToSubmissionImpl(const sol::stack_table& ids);
	template <typename TObj>
	SDrawElementsIndirectCommand DrawObjectGetCmdImpl(int id);
	template <typename TObj>
	static const SIndexAndCount GetDrawIndicesImpl(int id);
	template <typename TObj>
	static const SIndexAndCount GetDrawIndicesImpl(const TObj* obj);
private:
	VAO* vao = nullptr;

	std::shared_ptr<LuaVBOImpl> vertLuaVBO;
	std::shared_ptr<LuaVBOImpl> instLuaVBO;
	std::shared_ptr<LuaVBOImpl> indxLuaVBO;

	uint32_t baseInstance;
	std::vector<SDrawElementsIndirectCommand> submitCmds;
};

#endif //LUA_VAO_IMPL_H