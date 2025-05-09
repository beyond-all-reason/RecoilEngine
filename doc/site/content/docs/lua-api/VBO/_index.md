---
title: "VBO"
permalink: "docs/lua-api/VBO"
---
# class VBO





Vertex Buffer Object







---

## methods
---

### VBO.Delete
---
```lua
function VBO.Delete() ->  nil
```












### VBO.Define
---
```lua
function VBO.Define(
  size: number,
  attribs: (number|VBOAttributeDef[])
) ->  nil
```
@param `size` - The maximum number of elements this VBO can have.

@param `attribs` - When number, the maximum number of elements this VBO can have.

Otherwise, an array of arrays specifying the layout.






Specify the kind of VBO you will be using.

```lua
terrainVertexVBO:Define(numPoints, {{ id = 0, name = "pos", size = 2 }})
```

It is usually an array of vertex/color/uv data, but can also be an array of
instance uniforms.

If you want to specify multiple instances of something to render, you will
need to create another VBO, which also specifies the number of instances you
wish to render, and the size of the data passed to each instance.

If you want say 5 elements, and each element is defined in the layout:

```lua
{id = 0, name = "first", size = 1},{id = 1, name = "second", size = 2}}
```

Then the total size of your VBO will be `5 * (1 + 2)`.

They will be laid out consecutively: `[1,2],[1,2],[1,2],[1,2],[1,2]`.

This is important for when you call `VBO:Upload`, you need to make sure you
enter your data into the Lua array correctly.





@see VBO:Upload




### VBO.GetBufferSize
---
```lua
function VBO.GetBufferSize()
 -> elementsCount number
 -> bufferSizeInBytes number
 -> size number

```












### VBO.Upload
---
```lua
function VBO.Upload(
  vboData: number[],
  attributeIndex: integer?,
  elemOffset: integer?,
  luaStartIndex: integer?,
  luaFinishIndex: integer?
)
 -> indexData number[]
 -> elemOffset integer
 -> attrID (integer|(integer,integer...))

```
@param `vboData` - Array of values to upload into the VBO.

@param `attributeIndex` - (Default: `-1`)

If supplied with non-default value then the data from `vboData` will only be
used to upload the data to this particular attribute.

The whole `vboData` is expected to contain only attributeIndex data.

Otherwise all attributes get updated sequentially across attributes and elements.

@param `elemOffset` - (Default: `0`) The index in destination VBO (on GPU) at which storing begins.

@param `luaStartIndex` - (Default: `1`) The index of `vboData` at which copying begins.

@param `luaFinishIndex` - (Default: `#vboData`) The index of `vboData` at which copying ends.






Uploads data into the VBO.





@see VBO:Define




### VBO.Download
---
```lua
function VBO.Download(
  attributeIndex: integer?,
  elementOffset: integer?,
  elementCount: number?,
  forceGPURead: boolean?
) ->  unknown
```
@param `attributeIndex` - (Default: `-1`) when supplied with non-default value: only data
from specified attribute will be downloaded - otherwise all attributes are
downloaded

@param `elementOffset` - (Default: `0`) download data starting from this element

@param `elementCount` - number of elements to download

@param `forceGPURead` - (Default: `false`) force downloading the data from GPU buffer as opposed
to using shadow RAM buffer













### VBO.ModelsVBO
---
```lua
function VBO.ModelsVBO() -> buffer number?
```

@return `buffer` - size in bytes





Binds engine side vertex or index VBO containing models (units, features) data.

Also fills in VBO definition data as they're set for engine models (no need to do VBO:Define()).








### VBO.InstanceDataFromUnitDefIDs
---
```lua
function VBO.InstanceDataFromUnitDefIDs(
  unitDefIDs: (number|number[]),
  attrID: integer,
  teamIdOpt: integer?,
  elementOffset: integer?
)
 -> instanceData (number,number,number,number)
 -> elementOffset integer
 -> attrID integer

```





Fills in attribute data for each specified unitDefID

The instance data in that attribute will contain the offset to bind position
matrix in global matrices SSBO and offset to uniform buffer structure in
global per unit/feature uniform SSBO (unused for Unit/FeatureDefs), as
well as some auxiliary data ushc as draw flags and team index.

Data Layout:
```
SInstanceData:
   , matOffset{ matOffset_ }            // updated during the following draw frames
   , uniOffset{ uniOffset_ }            // updated during the following draw frames
   , info{ teamIndex, drawFlags, 0, 0 } // not updated during the following draw frames
   , aux1 { 0u }
```








### VBO.InstanceDataFromFeatureDefIDs
---
```lua
function VBO.InstanceDataFromFeatureDefIDs(
  featureDefIDs: (number|number[]),
  attrID: integer,
  teamIdOpt: integer?,
  elementOffset: integer?
)
 -> instanceData (number,number,number,number)
 -> elementOffset integer
 -> attrID integer

```





Fills in attribute data for each specified featureDefID

The instance data in that attribute will contain the offset to bind position
matrix in global matrices SSBO and offset to uniform buffer structure in
global per unit/feature uniform SSBO (unused for Unit/FeatureDefs), as
well as some auxiliary data ushc as draw flags and team index.

Data Layout
```
SInstanceData:
   , matOffset{ matOffset_ }            // updated during the following draw frames
   , uniOffset{ uniOffset_ }            // updated during the following draw frames
   , info{ teamIndex, drawFlags, 0, 0 } // not updated during the following draw frames
   , aux1 { 0u }
```








### VBO.InstanceDataFromUnitIDs
---
```lua
function VBO.InstanceDataFromUnitIDs(
  unitIDs: (number|number[]),
  attrID: integer,
  teamIdOpt: integer?,
  elementOffset: integer?
)
 -> instanceData (number,number,number,number)
 -> elementOffset integer
 -> attrID integer

```





Fills in attribute data for each specified unitID

The instance data in that attribute will contain the offset to bind position
matrix in global matrices SSBO and offset to uniform buffer structure in
global per unit/feature uniform SSBO (unused for Unit/FeatureDefs), as
well as some auxiliary data ushc as draw flags and team index.

Data Layout

```
SInstanceData:
   , matOffset{ matOffset_ }            // updated during the following draw frames
   , uniOffset{ uniOffset_ }            // updated during the following draw frames
   , info{ teamIndex, drawFlags, 0, 0 } // not updated during the following draw frames
   , aux1 { 0u }
```








### VBO.InstanceDataFromFeatureIDs
---
```lua
function VBO.InstanceDataFromFeatureIDs(
  featureIDs: (number|number[]),
  attrID: integer,
  teamIdOpt: integer?,
  elementOffset: integer?
)
 -> instanceData (number,number,number,number)
 -> elementOffset integer
 -> attrID integer

```





Fills in attribute data for each specified featureID

The instance data in that attribute will contain the offset to bind position
matrix in global matrices SSBO and offset to uniform buffer structure in
global per unit/feature uniform SSBO (unused for Unit/FeatureDefs), as
well as some auxiliary data ushc as draw flags and team index.








### VBO.MatrixDataFromProjectileIDs
---
```lua
function VBO.MatrixDataFromProjectileIDs(
  projectileIDs: (integer|integer[]),
  attrID: integer,
  teamIdOpt: integer?,
  elementOffset: integer?
)
 -> matDataVec number[]
 -> elemOffset integer
 -> attrID (integer|(integer,integer...))

```

@return `matDataVec` - 4x4 matrix












### VBO.BindBufferRange
---
```lua
function VBO.BindBufferRange(
  index: integer,
  elementOffset: integer?,
  elementCount: number?,
  target: number?
) -> bindingIndex integer
```
@param `index` - should be in the range between
`5 < index < GL_MAX_UNIFORM_BUFFER_BINDINGS` value (usually 31)

@param `target` - glEnum


@return `bindingIndex` - when successful, -1 otherwise





Bind a range within a buffer object to an indexed buffer target

Generally mimics
https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBindBufferRange.xhtml
except offset and size are specified in number of elements / element indices.








### VBO.UnbindBufferRange
---
```lua
function VBO.UnbindBufferRange(
  index: integer,
  elementOffset: integer?,
  elementCount: number?,
  target: number?
) -> bindingIndex number
```
@param `target` - glEnum


@return `bindingIndex` - when successful, -1 otherwise












### VBO.DumpDefinition
---
```lua
function VBO.DumpDefinition() ->  nil
```





Logs the definition of the VBO to the console








### VBO.GetID
---
```lua
function VBO.GetID() -> bufferID integer
```





Gets the OpenGL Buffer ID








### VBO.Delete
---
```lua
function VBO.Delete() ->  nil
```












### VBO.Define
---
```lua
function VBO.Define(
  size: number,
  attribs: (number|VBOAttributeDef[])
) ->  nil
```
@param `size` - The maximum number of elements this VBO can have.

@param `attribs` - When number, the maximum number of elements this VBO can have.

Otherwise, an array of arrays specifying the layout.






Specify the kind of VBO you will be using.

```lua
terrainVertexVBO:Define(numPoints, {{ id = 0, name = "pos", size = 2 }})
```

It is usually an array of vertex/color/uv data, but can also be an array of
instance uniforms.

If you want to specify multiple instances of something to render, you will
need to create another VBO, which also specifies the number of instances you
wish to render, and the size of the data passed to each instance.

If you want say 5 elements, and each element is defined in the layout:

```lua
{id = 0, name = "first", size = 1},{id = 1, name = "second", size = 2}}
```

Then the total size of your VBO will be `5 * (1 + 2)`.

They will be laid out consecutively: `[1,2],[1,2],[1,2],[1,2],[1,2]`.

This is important for when you call `VBO:Upload`, you need to make sure you
enter your data into the Lua array correctly.





@see VBO:Upload




### VBO.GetBufferSize
---
```lua
function VBO.GetBufferSize()
 -> elementsCount number
 -> bufferSizeInBytes number
 -> size number

```












### VBO.Upload
---
```lua
function VBO.Upload(
  vboData: number[],
  attributeIndex: integer?,
  elemOffset: integer?,
  luaStartIndex: integer?,
  luaFinishIndex: integer?
)
 -> indexData number[]
 -> elemOffset integer
 -> attrID (integer|(integer,integer...))

```
@param `vboData` - Array of values to upload into the VBO.

@param `attributeIndex` - (Default: `-1`)

If supplied with non-default value then the data from `vboData` will only be
used to upload the data to this particular attribute.

The whole `vboData` is expected to contain only attributeIndex data.

Otherwise all attributes get updated sequentially across attributes and elements.

@param `elemOffset` - (Default: `0`) The index in destination VBO (on GPU) at which storing begins.

@param `luaStartIndex` - (Default: `1`) The index of `vboData` at which copying begins.

@param `luaFinishIndex` - (Default: `#vboData`) The index of `vboData` at which copying ends.






Uploads data into the VBO.





@see VBO:Define




### VBO.Download
---
```lua
function VBO.Download(
  attributeIndex: integer?,
  elementOffset: integer?,
  elementCount: number?,
  forceGPURead: boolean?
) ->  unknown
```
@param `attributeIndex` - (Default: `-1`) when supplied with non-default value: only data
from specified attribute will be downloaded - otherwise all attributes are
downloaded

@param `elementOffset` - (Default: `0`) download data starting from this element

@param `elementCount` - number of elements to download

@param `forceGPURead` - (Default: `false`) force downloading the data from GPU buffer as opposed
to using shadow RAM buffer













### VBO.ModelsVBO
---
```lua
function VBO.ModelsVBO() -> buffer number?
```

@return `buffer` - size in bytes





Binds engine side vertex or index VBO containing models (units, features) data.

Also fills in VBO definition data as they're set for engine models (no need to do VBO:Define()).








### VBO.InstanceDataFromUnitDefIDs
---
```lua
function VBO.InstanceDataFromUnitDefIDs(
  unitDefIDs: (number|number[]),
  attrID: integer,
  teamIdOpt: integer?,
  elementOffset: integer?
)
 -> instanceData (number,number,number,number)
 -> elementOffset integer
 -> attrID integer

```





Fills in attribute data for each specified unitDefID

The instance data in that attribute will contain the offset to bind position
matrix in global matrices SSBO and offset to uniform buffer structure in
global per unit/feature uniform SSBO (unused for Unit/FeatureDefs), as
well as some auxiliary data ushc as draw flags and team index.

Data Layout:
```
SInstanceData:
   , matOffset{ matOffset_ }            // updated during the following draw frames
   , uniOffset{ uniOffset_ }            // updated during the following draw frames
   , info{ teamIndex, drawFlags, 0, 0 } // not updated during the following draw frames
   , aux1 { 0u }
```








### VBO.InstanceDataFromFeatureDefIDs
---
```lua
function VBO.InstanceDataFromFeatureDefIDs(
  featureDefIDs: (number|number[]),
  attrID: integer,
  teamIdOpt: integer?,
  elementOffset: integer?
)
 -> instanceData (number,number,number,number)
 -> elementOffset integer
 -> attrID integer

```





Fills in attribute data for each specified featureDefID

The instance data in that attribute will contain the offset to bind position
matrix in global matrices SSBO and offset to uniform buffer structure in
global per unit/feature uniform SSBO (unused for Unit/FeatureDefs), as
well as some auxiliary data ushc as draw flags and team index.

Data Layout
```
SInstanceData:
   , matOffset{ matOffset_ }            // updated during the following draw frames
   , uniOffset{ uniOffset_ }            // updated during the following draw frames
   , info{ teamIndex, drawFlags, 0, 0 } // not updated during the following draw frames
   , aux1 { 0u }
```








### VBO.InstanceDataFromUnitIDs
---
```lua
function VBO.InstanceDataFromUnitIDs(
  unitIDs: (number|number[]),
  attrID: integer,
  teamIdOpt: integer?,
  elementOffset: integer?
)
 -> instanceData (number,number,number,number)
 -> elementOffset integer
 -> attrID integer

```





Fills in attribute data for each specified unitID

The instance data in that attribute will contain the offset to bind position
matrix in global matrices SSBO and offset to uniform buffer structure in
global per unit/feature uniform SSBO (unused for Unit/FeatureDefs), as
well as some auxiliary data ushc as draw flags and team index.

Data Layout

```
SInstanceData:
   , matOffset{ matOffset_ }            // updated during the following draw frames
   , uniOffset{ uniOffset_ }            // updated during the following draw frames
   , info{ teamIndex, drawFlags, 0, 0 } // not updated during the following draw frames
   , aux1 { 0u }
```








### VBO.InstanceDataFromFeatureIDs
---
```lua
function VBO.InstanceDataFromFeatureIDs(
  featureIDs: (number|number[]),
  attrID: integer,
  teamIdOpt: integer?,
  elementOffset: integer?
)
 -> instanceData (number,number,number,number)
 -> elementOffset integer
 -> attrID integer

```





Fills in attribute data for each specified featureID

The instance data in that attribute will contain the offset to bind position
matrix in global matrices SSBO and offset to uniform buffer structure in
global per unit/feature uniform SSBO (unused for Unit/FeatureDefs), as
well as some auxiliary data ushc as draw flags and team index.








### VBO.MatrixDataFromProjectileIDs
---
```lua
function VBO.MatrixDataFromProjectileIDs(
  projectileIDs: (integer|integer[]),
  attrID: integer,
  teamIdOpt: integer?,
  elementOffset: integer?
)
 -> matDataVec number[]
 -> elemOffset integer
 -> attrID (integer|(integer,integer...))

```

@return `matDataVec` - 4x4 matrix












### VBO.BindBufferRange
---
```lua
function VBO.BindBufferRange(
  index: integer,
  elementOffset: integer?,
  elementCount: number?,
  target: number?
) -> bindingIndex integer
```
@param `index` - should be in the range between
`5 < index < GL_MAX_UNIFORM_BUFFER_BINDINGS` value (usually 31)

@param `target` - glEnum


@return `bindingIndex` - when successful, -1 otherwise





Bind a range within a buffer object to an indexed buffer target

Generally mimics
https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBindBufferRange.xhtml
except offset and size are specified in number of elements / element indices.








### VBO.UnbindBufferRange
---
```lua
function VBO.UnbindBufferRange(
  index: integer,
  elementOffset: integer?,
  elementCount: number?,
  target: number?
) -> bindingIndex number
```
@param `target` - glEnum


@return `bindingIndex` - when successful, -1 otherwise












### VBO.DumpDefinition
---
```lua
function VBO.DumpDefinition() ->  nil
```





Logs the definition of the VBO to the console








### VBO.GetID
---
```lua
function VBO.GetID() -> bufferID integer
```





Gets the OpenGL Buffer ID











