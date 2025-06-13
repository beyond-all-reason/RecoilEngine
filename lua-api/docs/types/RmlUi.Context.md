---
layout: default
title: Context
parent: Lua API
permalink: lua-api/types/Context
---

{% raw %}

# class Context



- namespace: RmlUi



Holds documents and a data model.
The Context class has no constructor; it must be instantiated through the CreateContext() function. It has the following functions and properties:

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Context.cpp#L283-L287" target="_blank">source</a>]







---

## methods
---

### Context.AddEventListener
---
```lua
function Context.AddEventListener(
  event: string,
  script: RmlUi.Element,
  element_context: boolean,
  in_capture_phase: boolean
)
```





Adds the inline Lua script, script, as an event listener to the context. element_context is an optional Element; if it is not None, then the script will be executed as if it was bound to that element.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Context.cpp#L292-L299" target="_blank">source</a>]








### Context.CreateDocument
---
```lua
function Context.CreateDocument(tag: string) ->  RmlUi.Document
```





Creates a new document with the tag name of tag.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Context.cpp#L301-L306" target="_blank">source</a>]








### Context.LoadDocument
---
```lua
function Context.LoadDocument(document_path: string) ->  RmlUi.Document
```





Attempts to load a document from the RML file found at document_path. If successful, the document will be returned with a reference count of one.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Context.cpp#L312-L317" target="_blank">source</a>]








### Context.GetDocument
---
```lua
function Context.GetDocument(name: string)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Context.cpp#L328-L331" target="_blank">source</a>]








### Context.Render
---
```lua
function Context.Render() ->  boolean
```





Renders the context.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Context.cpp#L333-L337" target="_blank">source</a>]








### Context.UnloadAllDocuments
---
```lua
function Context.UnloadAllDocuments()
```





Closes all documents currently loaded with the context.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Context.cpp#L339-L342" target="_blank">source</a>]








### Context.UnloadDocument
---
```lua
function Context.UnloadDocument(document: RmlUi.Document)
```





Unloads a specific document within the context.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Context.cpp#L344-L348" target="_blank">source</a>]








### Context.Update
---
```lua
function Context.Update() ->  boolean
```





Updates the context.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Context.cpp#L350-L354" target="_blank">source</a>]








### Context.OpenDataModel
---
```lua
function Context.OpenDataModel(
  name: string,
  model: T,
  widget: table
) ->  SolLuaDataModel<T>
```





Create a new data model from a base table `model` and register to the context. The model table is copied.
Note that `widget` does not actually have to be a widget; it can be any table. This table can be accessed in widgets like `<button class="mode-button" onclick="widget:SetCamMode()">Set Dolly Mode</button>`. Also note that your data model is inaccessible in `onx` properties.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Context.cpp#L356-L365" target="_blank">source</a>]








### Context.RemoveDataModel
---
```lua
function Context.RemoveDataModel(name: string)
```





Removes a data model from the context.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Context.cpp#L367-L371" target="_blank">source</a>]








### Context.ProcessMouseMove
---
```lua
function Context.ProcessMouseMove(position: RmlUi.Vector2f) ->  boolean
```





Processes a mouse move event.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Context.cpp#L373-L378" target="_blank">source</a>]








### Context.ProcessMouseButtonDown
---
```lua
function Context.ProcessMouseButtonDown(
  button: RmlUi.MouseButton,
  key_modifier_state: integer
) ->  boolean
```





Processes a mouse button down event.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Context.cpp#L380-L386" target="_blank">source</a>]








### Context.ProcessMouseButtonUp
---
```lua
function Context.ProcessMouseButtonUp(
  button: RmlUi.MouseButton,
  key_modifier_state: integer
) ->  boolean
```





Processes a mouse button up event.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Context.cpp#L388-L394" target="_blank">source</a>]








### Context.ProcessMouseWheel
---
```lua
function Context.ProcessMouseWheel(
  delta: (RmlUi.Vector2f|number),
  key_modifier_state: integer
) ->  boolean
```





Processes a mouse wheel event.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Context.cpp#L396-L402" target="_blank">source</a>]








### Context.ProcessMouseLeave
---
```lua
function Context.ProcessMouseLeave() ->  boolean
```





Processes a mouse leave event.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Context.cpp#L406-L410" target="_blank">source</a>]








### Context.IsMouseInteracting
---
```lua
function Context.IsMouseInteracting() ->  boolean
```





Returns true if the mouse is currently interacting with the context, false if not.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Context.cpp#L412-L416" target="_blank">source</a>]








### Context.ProcessKeyDown
---
```lua
function Context.ProcessKeyDown(
  key: RmlUi.key_identifier,
  key_modifier_state: integer
) ->  boolean
```





Processes a key down event.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Context.cpp#L418-L424" target="_blank">source</a>]








### Context.ProcessKeyUp
---
```lua
function Context.ProcessKeyUp(
  key: RmlUi.key_identifier,
  key_modifier_state: integer
) ->  boolean
```





Processes a key up event.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Context.cpp#L426-L432" target="_blank">source</a>]








### Context.ProcessTextInput
---
```lua
function Context.ProcessTextInput(text: string) ->  boolean
```





Processes a text input event.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Context.cpp#L434-L439" target="_blank">source</a>]








### Context.EnableMouseCursor
---
```lua
function Context.EnableMouseCursor(enable: boolean)
```





Enables or disables the mouse cursor for the context.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Context.cpp#L442-L446" target="_blank">source</a>]








### Context.ActivateTheme
---
```lua
function Context.ActivateTheme(
  theme_name: string,
  activate: boolean
)
```





Activates a theme for the context.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Context.cpp#L448-L453" target="_blank">source</a>]








### Context.IsThemeActive
---
```lua
function Context.IsThemeActive(theme_name: string) ->  boolean
```





Returns true if the theme is active, false if not.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Context.cpp#L455-L460" target="_blank">source</a>]








### Context.GetElementAtPoint
---
```lua
function Context.GetElementAtPoint(
  point: RmlUi.Vector2f,
  ignore: RmlUi.Element?
) ->  RmlUi.Element
```





Returns the element at the point specified by point.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Context.cpp#L462-L468" target="_blank">source</a>]








### Context.PullDocumentToFront
---
```lua
function Context.PullDocumentToFront(document: RmlUi.Document)
```





Pulls the document to the front of the context.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Context.cpp#L470-L474" target="_blank">source</a>]








### Context.PushDocumentToBack
---
```lua
function Context.PushDocumentToBack(document: RmlUi.Document)
```





Pushes the document to the back of the context.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Context.cpp#L476-L480" target="_blank">source</a>]











## fields
---

### Context.dimensions
---
```lua
Context.dimensions : RmlUi.Vector2i {
    x: integer,
    y: integer,
    magnitude: number,
}
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Context.cpp#L484-L484" target="_blank">source</a>]








### Context.dp_ratio
---
```lua
Context.dp_ratio : number
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Context.cpp#L486-L486" target="_blank">source</a>]








### Context.documents
---
```lua
Context.documents : RmlUi.Document[]
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Context.cpp#L489-L489" target="_blank">source</a>]








### Context.focus_element
---
```lua
Context.focus_element : RmlUi.Element {
    class_name: string,
    id: string,
    inner_rml: string,
    scroll_left: integer,
    scroll_top: integer,
    attributes: RmlUi.ElementAttributesProxy,
    child_nodes: RmlUi.ElementChildNodesProxy,
    client_left: integer,
    client_height: integer,
    client_top: integer,
    client_width: integer,
    first_child: RmlUi.Element?,
    ...
}
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Context.cpp#L491-L491" target="_blank">source</a>]








### Context.hover_element
---
```lua
Context.hover_element : RmlUi.Element {
    class_name: string,
    id: string,
    inner_rml: string,
    scroll_left: integer,
    scroll_top: integer,
    attributes: RmlUi.ElementAttributesProxy,
    child_nodes: RmlUi.ElementChildNodesProxy,
    client_left: integer,
    client_height: integer,
    client_top: integer,
    client_width: integer,
    first_child: RmlUi.Element?,
    ...
}
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Context.cpp#L493-L493" target="_blank">source</a>]








### Context.name
---
```lua
Context.name : string
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Context.cpp#L495-L495" target="_blank">source</a>]








### Context.root_element
---
```lua
Context.root_element : RmlUi.Element {
    class_name: string,
    id: string,
    inner_rml: string,
    scroll_left: integer,
    scroll_top: integer,
    attributes: RmlUi.ElementAttributesProxy,
    child_nodes: RmlUi.ElementChildNodesProxy,
    client_left: integer,
    client_height: integer,
    client_top: integer,
    client_width: integer,
    first_child: RmlUi.Element?,
    ...
}
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Context.cpp#L497-L497" target="_blank">source</a>]










{% endraw %}