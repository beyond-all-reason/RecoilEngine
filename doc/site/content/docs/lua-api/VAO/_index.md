---
title: "VAO"
permalink: "docs/lua-api/VAO"
---
# class VAO





Vertex Array Object







---

## methods
---

### VAO.Delete
---
```lua
function VAO.Delete() ->  nil
```












### VAO.AttachVertexBuffer
---
```lua
function VAO.AttachVertexBuffer(vbo: VBO) ->  nil
```





Attaches a VBO to be used as a vertex buffer








### VAO.AttachInstanceBuffer
---
```lua
function VAO.AttachInstanceBuffer(vbo: VBO) ->  nil
```





Attaches a VBO to be used as an instance buffer








### VAO.AttachIndexBuffer
---
```lua
function VAO.AttachIndexBuffer(vbo: VBO) ->  nil
```





Attaches a VBO to be used as an index buffer








### VAO.DrawArrays
---
```lua
function VAO.DrawArrays(
  glEnum: number,
  vertexCount: number?,
  vertexFirst: number?,
  instanceCount: number?,
  instanceFirst: number?
) ->  nil
```
@param `glEnum` - primitivesMode













### VAO.DrawElements
---
```lua
function VAO.DrawElements(
  glEnum: number,
  drawCount: number?,
  baseIndex: number?,
  instanceCount: number?,
  baseVertex: number?,
  baseInstance: number?
) ->  nil
```
@param `glEnum` - primitivesMode













### VAO.AddUnitsToSubmission
---
```lua
function VAO.AddUnitsToSubmission(unitIDs: (number|number[])) -> submittedCount number
```












### VAO.AddFeaturesToSubmission
---
```lua
function VAO.AddFeaturesToSubmission(featureIDs: (number|number[])) -> submittedCount number
```












### VAO.AddUnitDefsToSubmission
---
```lua
function VAO.AddUnitDefsToSubmission(unitDefIDs: (number|number[])) -> submittedCount number
```












### VAO.AddFeatureDefsToSubmission
---
```lua
function VAO.AddFeatureDefsToSubmission(featureDefIDs: (number|number[])) -> submittedCount number
```












### VAO.RemoveFromSubmission
---
```lua
function VAO.RemoveFromSubmission(index: number) ->  nil
```












### VAO.Submit
---
```lua
function VAO.Submit() ->  nil
```












### VAO.Delete
---
```lua
function VAO.Delete() ->  nil
```












### VAO.AttachVertexBuffer
---
```lua
function VAO.AttachVertexBuffer(vbo: VBO) ->  nil
```





Attaches a VBO to be used as a vertex buffer








### VAO.AttachInstanceBuffer
---
```lua
function VAO.AttachInstanceBuffer(vbo: VBO) ->  nil
```





Attaches a VBO to be used as an instance buffer








### VAO.AttachIndexBuffer
---
```lua
function VAO.AttachIndexBuffer(vbo: VBO) ->  nil
```





Attaches a VBO to be used as an index buffer








### VAO.DrawArrays
---
```lua
function VAO.DrawArrays(
  glEnum: number,
  vertexCount: number?,
  vertexFirst: number?,
  instanceCount: number?,
  instanceFirst: number?
) ->  nil
```
@param `glEnum` - primitivesMode













### VAO.DrawElements
---
```lua
function VAO.DrawElements(
  glEnum: number,
  drawCount: number?,
  baseIndex: number?,
  instanceCount: number?,
  baseVertex: number?,
  baseInstance: number?
) ->  nil
```
@param `glEnum` - primitivesMode













### VAO.AddUnitsToSubmission
---
```lua
function VAO.AddUnitsToSubmission(unitIDs: (number|number[])) -> submittedCount number
```












### VAO.AddFeaturesToSubmission
---
```lua
function VAO.AddFeaturesToSubmission(featureIDs: (number|number[])) -> submittedCount number
```












### VAO.AddUnitDefsToSubmission
---
```lua
function VAO.AddUnitDefsToSubmission(unitDefIDs: (number|number[])) -> submittedCount number
```












### VAO.AddFeatureDefsToSubmission
---
```lua
function VAO.AddFeatureDefsToSubmission(featureDefIDs: (number|number[])) -> submittedCount number
```












### VAO.RemoveFromSubmission
---
```lua
function VAO.RemoveFromSubmission(index: number) ->  nil
```












### VAO.Submit
---
```lua
function VAO.Submit() ->  nil
```















