+++
title = "Making your own shaders"
author = "Slashscreen"
+++

Recoil comes with some premade shaders that cover the majority of use cases, but there are some occasions in which you need custom shaders. Luckily, Recoil lets you to just about whatever you want. For the uninitiated, though, it can be a little confusing; that's where this guide comes in. 

## Step 1: The Shader

First thing's first, let's make a basic shader (For GL4).

**Vertex Shader (`shader.vert.glsl`):**
```glsl
#version 460

//##UBO##
// We'll talk about this later

in vec4 instancePosition;
in vec4 pos; // Note: To project properly, it should be a vec4 with the w value being 1.0
in uvec3 color;
in vec2 uv;

out DataVS { // Interpolated for each vertex
    vec2 texCoord;
};

void main() {
    gl_Position = cameraViewProj * (instancePosition + pos);
    texCoord = uv;
}
```

**Fragment Shader (`shader.frag.glsl`):**
```glsl
#version 460

uniform sampler2D inputTexture;

in DataVS {
    vec2 texCoord;
};

out vec4 colorOut; // This is the final fragment color

void main() {
    colorOut = texture(inputTexture, texCoord);
}
```


Creating a basic shader in Lua is simple.

```lua
local ubo_defs = gl.GetEngineUniformBuffer()
shader = gl.CreateShader({
    fragment = VFS.LoadFile("path/to/shader.frag.glsl"):gsub("//##UBO##", ubo_defs), -- again we will address this later
    vertex = VFS.LoadFile("path/to/shader.vert.glsl"):gsub("//##UBO##", ubo_defs),
})
```

Easy as. This assumes that the shader is in a separate file, which I believe is best practice, since it lets you use your IDE to help you write the shader. `fragment` and `vertex` take strings, though, and so you can just as easily write the shader directly into your Lua file, and indeed, many widgets opt to do that. `VFS.LoadFile` will load the file as a string, so it's the same thing.

But we aren't at a working place yet.

## Step 2: The Uniforms

There are some uniforms in these shaders. Some data is provided by the engine in a Uniform Buffer Object, but others have to be provided by you. 

### Engine Uniforms

All shaders have access to a Uniform Buffer Object containing information about the game world. The shape is as follows:

```glsl
layout(std140, binding = 0) uniform UniformMatrixBuffer {
	mat4 screenView;
	mat4 screenProj;
	mat4 screenViewProj;

	mat4 cameraView;
	mat4 cameraProj;
	mat4 cameraViewProj;
	mat4 cameraBillboardView;

	mat4 cameraViewInv;
	mat4 cameraProjInv;
	mat4 cameraViewProjInv;

	mat4 shadowView;
	mat4 shadowProj;
	mat4 shadowViewProj;

	mat4 reflectionView;
	mat4 reflectionProj;
	mat4 reflectionViewProj;

	mat4 orthoProj01;

	// transforms for [0] := Draw, [1] := DrawInMiniMap, [2] := Lua DrawInMiniMap
	mat4 mmDrawView; //world to MM
	mat4 mmDrawProj; //world to MM
	mat4 mmDrawViewProj; //world to MM

	mat4 mmDrawIMMView; //heightmap to MM
	mat4 mmDrawIMMProj; //heightmap to MM
	mat4 mmDrawIMMViewProj; //heightmap to MM

	mat4 mmDrawDimView; //mm dims
	mat4 mmDrawDimProj; //mm dims
	mat4 mmDrawDimViewProj; //mm dims
};

layout(std140, binding = 1) uniform UniformParamsBuffer {
	vec3 rndVec3; //new every draw frame.
	uint renderCaps; //various render booleans

	vec4 timeInfo; //gameFrame, drawSeconds, interpolated(unsynced)GameSeconds(synced), frameTimeOffset
	vec4 viewGeometry; //vsx, vsy, vpx, vpy
	vec4 mapSize; //xz, xzPO2
	vec4 mapHeight; //height minCur, maxCur, minInit, maxInit

	vec4 fogColor; //fog color
	vec4 fogParams; //fog {start, end, 0.0, scale}

	vec4 sunDir; // (sky != nullptr) ? sky->GetLight()->GetLightDir() : float4(/*map default*/ 0.0f, 0.447214f, 0.894427f, 1.0f);

	vec4 sunAmbientModel;
	vec4 sunAmbientMap;
	vec4 sunDiffuseModel;
	vec4 sunDiffuseMap;
	vec4 sunSpecularModel; // float4{ sunLighting->modelSpecularColor.xyz, sunLighting->specularExponent };
	vec4 sunSpecularMap; //  float4{ sunLighting->groundSpecularColor.xyz, sunLighting->specularExponent };

	vec4 shadowDensity; //  float4{ sunLighting->groundShadowDensity, sunLighting->modelShadowDensity, 0.0, 0.0 };

	vec4 windInfo; // windx, windy, windz, windStrength
	vec2 mouseScreenPos; //x, y. Screen space.
	uint mouseStatus; // bits 0th to 32th: LMB, MMB, RMB, offscreen, mmbScroll, locked
	uint mouseUnused;
	vec4 mouseWorldPos; //x,y,z; w=0 -- offmap. Ignores water, doesn't ignore units/features under the mouse cursor

	vec4 teamColor[255]; //all team colors
};
```

To include this in your shader, it's better to append this to the beginning of your shader on the lua side with [`gl.GetEngineUniformBuffer()`]({{% ref "docs/lua-api/gl/#glgetengineuniformbufferdef" %}}), like `fragment = VFS.LoadFile("path/to/shader.frag.glsl"):gsub("//##UBO##", gl.GetEngineUniformBuffer())`. We use the gsub here to put UBO in the middle of the code, rather than just prepend engine UBO in front of the shader, to make sure it's after the `#version` stuff glsl expects to be at the beginning. If you plan to use them in your shader, make sure to add `//##UBO##` as a comment up near the beginning. This way, if something changes, you won't have to update all of your scripts (but we will still have to update this article).

### Custom Uniforms

Often times, you want to pass in your own uniform data. To do that, we have to define uniform bindings when creating our shader. Our fragment shader has an input texture, so let's put that in:

```lua
shader = gl.CreateShader({
    fragment = VFS.LoadFile("path/to/shader.frag.glsl"),
    vertex = VFS.LoadFile("path/to/shader.vert.glsl"),
    uniformInt = {
        inputTexture = 0, -- we'll get to this
    }
})
```

The `ShaderParams` table has a number of uniform input fields; these are maps mapping the uniform name as it appears in the shader to a table of the data.

#### Pass In Vec4

For vectors, the data is simply an array.

```lua
uniformFloat = {
    myVec4Uniform = {1.0, 1.0, 1.0, 1.0},
}
```

#### Pass In Textures

This one is a little trickier, but it's easy once you figure it out. Your sampler is actually passed in as an integer index:

```lua
local MY_TEXTURE_INDEX = 0
uniformInt = {
        myTexture = MY_TEXTURE_INDEX,
    }
```

Then, we have to actually upload the texture data.

```lua
local texture_sampling_mode = '' -- you can set different sampling modes but I am not sure where the documentation is for this
local filepath = texture_sampling_mode .. "path/to/my_texture.png"
gl.Texture(filepath, MY_TEXTURE_INDEX)
```

And you should be good to go.

There are some built-in path values and format examples below, for further information see the [referencing textures article]({{% ref "articles/texture-reference-strings" %}})

| Name | Role |
| --- | --- |
| $heightmap | ground heightmap |
| $reflection | reflection cube map |
| $specular | specular cube map |
| $shadow | shadowmap | 
| !x | Lua generated texture x |
| %x:0 | unitDef x s3o tex1 |
| %-x:1 | featureDef x s3o tex2 |
| #x | unitDef x build pic  |
| ^x | unitDef x icon |

### Editing Uniforms During Runtime

To change uniforms during runtime, you will have to grab a uniform location. Luckily, it's easy to do that.

```lua
local uniform_location = gl.GetUniformLocation(shader, "myUniformName")
```

Then, to change the uniform, you use the location:

```lua
gl.Uniform(uniform_location, 1.0, 0.0, 1.0, 0.0)
```

There are a couple of these `gl.UniformX` functions for floats, integers, etc, but they all have in common that you can put in up to 4 values, representing the maximum vector dimensions of 4.  

There are a few caveats, though:
- The shader must be active (we'll get to that)
- Uniforms can only be updated during a `draw` call (`widget:DrawWorld` is usually a safe bet).

## Step 3: The Buffer

We [already have an article on buffers](docs/guides/rendering/lua-vbo-vao), but let's do a quick run-through.

A Vertex Buffer Object (VBO) is a linear buffer of arbitrary data. This is the actual data we will edit.
A Vertex *Array* Object (VAO) bundles all your buffers together.

### Creation

Let's create some. Note that any of these can be skipped apart from the VAO if you don't need them.

```lua
local my_vao = gl.GetVAO() -- first, create a VAO

local vert_vbo = gl.GetVBO(GL.ARRAY_BUFFER, true) --empty VBO, "GL.ARRAY_BUFFER" means it's either vertex or instance buffer, "true" means this buffer will be optimized by GL driver for frequent updates. Here by the variable name you can guess it's supposed to be vertex buffer

local instance_vbo = gl.GetVBO(GL.ARRAY_BUFFER, true)

local index_vbo = gl.GetVBO(GL.ELEMENT_ARRAY_BUFFER, false) -- empty index buffer, not going to be frequently updated ("false").
```

### Definition

Next, we need to define the shapes of some of these buffers.

```lua
--[[ 
    Here, we define the vertex buffer.
    With "8", we will be holding 8 elements, one for each vertex.
    Next ,we define the actual shape of the vertex.
    To resize this, we would have to remake a buffer, remove the old one, copy the data, then attach the new one.
]]
vert_vbo:Define(8, {
    {id = 0, name = "pos", size = 4}, -- vec4. The default type is a float. Size is the number of dimensions in the vector; in this case, 4.
    {id = 1, name = "color", type=GL.UNSIGNED_BYTE, normalized = true, size = 3}, -- a uvec3 of bytes. Unused in our shader, but for demonstration purposes.
    {id = 2, name = "uv", size = 2},
})

--[[
    Here, we define the instance buffer; Custom data for each instance of an object we want to draw.
    We want to draw 2 objects.
    Each instance gets a vec4 instancePosition.
]]
instance_vbo:Define(2, {
    {id = 0, name = "instancePosition", size = 4},
})

-- The index buffer is just a buffer of indices, which are u32s. the 6 * 6 comes from the indices we will use to draw a cube.
index_vbo:Define(6 * 6, GL.UNSIGNED_INT)

-- Now, we attach the buffers.
my_vao:AttachVertexBuffer(vert_vbo)
my_vao:AttachInstanceBuffer(instance_vbo)
my_vao:AttachIndexBuffer(index_vbo)
```

### Data Uploading

Data is loaded using `VBO:Upload`. There are some options for it, but for now we will see he basic usage. Data is a stream of unstructured data, so it's not as intuitive as everything else.

```lua
vert_vbo:Upload({
    -- element 1
    0.0, 0.0, 0.0, 1.0, -- goes into "pos". Remember the w being 1.0
    255, 255, 255, -- goes into "color"
    0.0, 0.0, -- "uv"
    -- element 2
    1.0, 0.0, 0.0, 1.0, -- goes into "pos"
    255, 255, 255, -- goes into "color"
    1.0, 0.0, -- "uv"
    -- ... etc
})

instance_vbo:Upload({
    -- element 1
    0.0, 0.0, 0.0, 1.0, -- "position"
    -- element 2
    100.0, 0.0, 100.0, 1.0, -- "position"
})

index_vbo:Upload({
    0, 1, 2, --one triangle
    1, 2, 3, --second triangle
    -- etc
})
```

## Step 4: Error Handling

Unfortunately, shaders tend to fail silently by default. Lucky for us, there's an idiom to get some feedback for when something has gone awry:

```lua
if shader == nil then
    local log = gl.GetShaderLog()
    Spring.Echo("Shader log: \n" .. log)
    if #log == 0 then
        Spring.Echo("Looks like shader linking failed. Make sure your in and out blocks match.")
    end
end
```

Walking through this:

- `gl.CreateShader` returns an `integer?`, where it's `nil` when something bad has happened. We can use that to check if we need to do some debugging.
- `gl.GetShaderLog` will get a string with any shader compilation errors. Echoing this into the game output can help you get some errors that your IDE might not have caught.
- If the log is empty and the shader is nil, there are no compilation errors, but the shader failed to link properly. This is often caused by an "in/out block mismatch". To fix this, the `out` variables of your vertex shader and the `in` variables of your fragment shader (also referred to as "varyings") should match.

## Step 5: Drawing

Now, let's draw. GL commands are done in an immediate-mode style, so to draw, we have to set the shader. 

```lua
gl.UseShader(shader)
someVAO:DrawElements(GL.TRIANGLES, numberOfIndices, indexOfFirstIndex, numberOfInstances, baseVertex) -- Draw with index buffer
gl.UseShader(0)
```

## Step 6: Some Optimizations And Best Practices

Here's a few tips for speeding up some hot loops:

- When moving stuff back and forth from the GPU often, it's best to try to pack things in strides of 128 bytes; a `vec4`. You can do some clever packing to move multiple values in one trip:

```glsl

in vec4 positionAndHealth;

void main() {
    vec4 position = vec4(positionAndHealth.xyz, 1.0); // for projection, positions should be a vec4, and w should be 1.0 so matrix multiplication works properly
    float health = positionAndHealth.w;
}
```

- This is more of a Lua optimization trick, but with rendering stuff, you run into hot loops very often, and should optimize accordingly (note that Recoil does *not* use LuaJIT, so these will not be jitted away). An easy way to do this is to pull the most common functions into the local scope to reduce lookup times:

```lua
local glUseShader = gl.UseShader
local glTexture = gl.Texture
local glUniformInt = gl.UniformInt
```
