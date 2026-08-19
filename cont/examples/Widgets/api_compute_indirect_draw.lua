--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    api_compute_indirect_draw.lua
--  brief:   Compute-written instance and indirect command buffer example
--  author:  RecoilEngine contributors
--
--  Demonstrates:
--    - Writing instance data and DrawElementsIndirectCommand from a compute shader
--    - Applying command and shader-storage memory barriers before drawing
--    - Drawing indexed instances without a conventional instance attribute VBO
--
--  License: GNU GPL, v2 or later
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:GetInfo()
	return {
		name = "Compute Indirect Draw API Example",
		desc = "Validates compute-written instance data and indexed indirect drawing",
		author = "RecoilEngine contributors",
		date = "2026",
		license = "GNU GPL, v2 or later",
		layer = 0,
		enabled = false,
	}
end

local glCreateShader = gl.CreateShader
local glDeleteShader = gl.DeleteShader
local glDispatchCompute = gl.DispatchCompute
local glGetShaderLog = gl.GetShaderLog
local glGetVAO = gl.GetVAO
local glGetVBO = gl.GetVBO
local glUseShader = gl.UseShader

local GL_ARRAY_BUFFER = GL.ARRAY_BUFFER
local GL_COMMAND_BARRIER_BIT = GL.COMMAND_BARRIER_BIT
local GL_ELEMENT_ARRAY_BUFFER = GL.ELEMENT_ARRAY_BUFFER
local GL_FLOAT_VEC4 = GL.FLOAT_VEC4
local GL_SHADER_STORAGE_BARRIER_BIT = GL.SHADER_STORAGE_BARRIER_BIT
local GL_SHADER_STORAGE_BUFFER = GL.SHADER_STORAGE_BUFFER
local GL_TRIANGLES = GL.TRIANGLES
local GL_UNSIGNED_INT_VEC4 = GL.UNSIGNED_INT_VEC4
local GL_UNSIGNED_SHORT = GL.UNSIGNED_SHORT

local INSTANCE_BUFFER_BINDING = 4
local COMMAND_BUFFER_BINDING = 5
local INSTANCE_COUNT = 4

local renderShader = nil
local computeShader = nil
local vertexVBO = nil
local indexVBO = nil
local instanceVBO = nil
local commandVBO = nil
local vao = nil

local renderVertexSource = [[
#version 430 core

layout(location = 0) in vec2 vertexPosition;

layout(std430, binding = 4) readonly buffer InstanceBuffer {
	vec4 instanceOffsets[];
};

flat out vec3 instanceColor;

const vec3 colors[4] = vec3[4](
	vec3(0.95, 0.25, 0.20),
	vec3(0.20, 0.80, 0.35),
	vec3(0.25, 0.55, 1.00),
	vec3(0.95, 0.75, 0.15)
);

void main()
{
	vec2 position = vertexPosition * 0.45 + instanceOffsets[gl_InstanceID].xy;
	gl_Position = vec4(position, 0.0, 1.0);
	instanceColor = colors[gl_InstanceID];
}
]]

local renderFragmentSource = [[
#version 430 core

flat in vec3 instanceColor;
out vec4 fragmentColor;

void main()
{
	fragmentColor = vec4(instanceColor, 1.0);
}
]]

local computeSource = [[
#version 430 core

layout(local_size_x = 4, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = 4) writeonly buffer InstanceBuffer {
	vec4 instanceOffsets[];
};

layout(std430, binding = 5) buffer CommandBuffer {
	uint count;
	uint instanceCount;
	uint firstIndex;
	int baseVertex;
	uint baseInstance;
};

const vec2 offsets[4] = vec2[4](
	vec2(-0.55, -0.35),
	vec2( 0.10, -0.35),
	vec2(-0.55,  0.30),
	vec2( 0.10,  0.30)
);

void main()
{
	uint instanceID = gl_GlobalInvocationID.x;
	instanceOffsets[instanceID] = vec4(offsets[instanceID], 0.0, 0.0);

	if (instanceID == 0u) {
		count = 3u;
		instanceCount = 4u;
		firstIndex = 0u;
		baseVertex = 0;
		baseInstance = 0u;
	}
}
]]

local function freeResources()
	if vao then
		vao:Delete()
		vao = nil
	end

	if commandVBO then
		commandVBO:Delete()
	end
	if instanceVBO then
		instanceVBO:Delete()
	end
	if indexVBO then
		indexVBO:Delete()
	end
	if vertexVBO then
		vertexVBO:Delete()
	end
	commandVBO = nil
	instanceVBO = nil
	indexVBO = nil
	vertexVBO = nil

	if computeShader then
		glDeleteShader(computeShader)
		computeShader = nil
	end
	if renderShader then
		glDeleteShader(renderShader)
		renderShader = nil
	end
end

local function fail(reason)
	Spring.Echo("Compute Indirect Draw API Example: " .. reason)
	freeResources()
	widgetHandler:RemoveWidget()
end

local function initResources()
	renderShader = glCreateShader({
		vertex = renderVertexSource,
		fragment = renderFragmentSource,
	})
	if not renderShader then
		return false, "render shader failed: " .. (glGetShaderLog() or "unknown error")
	end

	computeShader = glCreateShader({ compute = computeSource })
	if not computeShader then
		return false, "compute shader failed: " .. (glGetShaderLog() or "unknown error")
	end

	vertexVBO = glGetVBO(GL_ARRAY_BUFFER, false)
	indexVBO = glGetVBO(GL_ELEMENT_ARRAY_BUFFER, false)
	instanceVBO = glGetVBO(GL_SHADER_STORAGE_BUFFER, true)
	commandVBO = glGetVBO(GL_SHADER_STORAGE_BUFFER, true)
	vao = glGetVAO()

	if not (vertexVBO and indexVBO and instanceVBO and commandVBO and vao) then
		return false, "required GL4 VAO/VBO functionality is unavailable"
	end

	vertexVBO:Define(3, {
		{ id = 0, name = "vertexPosition", size = 2 },
	})
	vertexVBO:Upload({
		-0.50, -0.40,
		 0.50, -0.40,
		 0.00,  0.50,
	})

	indexVBO:Define(3, GL_UNSIGNED_SHORT)
	indexVBO:Upload({ 0, 1, 2 })

	instanceVBO:Define(INSTANCE_COUNT, {
		{ id = 0, name = "instanceOffsets", size = 1, type = GL_FLOAT_VEC4 },
	})

	-- Two uvec4 elements provide 32 bytes. The command occupies the first 20.
	commandVBO:Define(2, {
		{ id = 0, name = "indirectCommand", size = 1, type = GL_UNSIGNED_INT_VEC4 },
	})

	vao:AttachVertexBuffer(vertexVBO)
	vao:AttachIndexBuffer(indexVBO)

	return true
end

function widget:Initialize()
	if not (glCreateShader and glDispatchCompute and glGetVAO and glGetVBO) then
		fail("required GL4 API is unavailable")
		return
	end

	local success, reason = initResources()
	if not success then
		fail(reason)
	end
end

function widget:Shutdown()
	freeResources()
end

function widget:DrawScreen()
	instanceVBO:BindBufferRange(INSTANCE_BUFFER_BINDING)
	commandVBO:BindBufferRange(COMMAND_BUFFER_BINDING)

	glUseShader(computeShader)
	glDispatchCompute(
		1, 1, 1,
		GL_COMMAND_BARRIER_BIT + GL_SHADER_STORAGE_BARRIER_BIT
	)
	glUseShader(0)

	gl.DepthTest(false)
	gl.Blending(false)
	glUseShader(renderShader)
	vao:DrawElementsIndirect(GL_TRIANGLES, commandVBO)
	glUseShader(0)

	commandVBO:UnbindBufferRange(COMMAND_BUFFER_BINDING)
	instanceVBO:UnbindBufferRange(INSTANCE_BUFFER_BINDING)
end
