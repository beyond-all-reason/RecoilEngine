---
layout: default
title: RmlUi
parent: Lua API
permalink: lua-api/globals/RmlUi
---

{% raw %}

# global RmlUi


Global functions for Recoil's RmlUi implementation.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/RmlSolLua.cpp#L69-L72" target="_blank">source</a>]







---

## methods
---

### RmlUi.CreateContext
---
```lua
function RmlUi.CreateContext(name: string) -> nil RmlUi.Context?
```

@return `nil` - if failed.





Create a new context.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Global.cpp#L135-L142" target="_blank">source</a>]








### RmlUi.RemoveContext
---
```lua
function RmlUi.RemoveContext(context: (string|RmlUi.Context))
```





Remove a context.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Global.cpp#L152-L158" target="_blank">source</a>]








### RmlUi.LoadFontFace
---
```lua
function RmlUi.LoadFontFace(
  file_path: string,
  fallback: boolean?,
  weight: RmlUi.font_weight?
) -> success boolean
```





Load a font face.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Global.cpp#L163-L172" target="_blank">source</a>]








### RmlUi.GetContext
---
```lua
function RmlUi.GetContext(name: string) -> nil RmlUi.Context?
```

@return `nil` - if failed.





Get a context by name.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Global.cpp#L179-L186" target="_blank">source</a>]








### RmlUi.RegiserEventType
---
```lua
function RmlUi.RegiserEventType(
  event_type: string,
  interruptible: boolean?,
  bubbles: boolean?,
  default_phase: RmlUi.default_action_phase?
) ->  RmlUi.EventID
```





Register a new event type.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Global.cpp#L228-L238" target="_blank">source</a>]








### RmlUi.AddTranslationString
---
```lua
function RmlUi.AddTranslationString(
  key: string,
  translation: string
) -> success boolean
```





Add a translation string.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Global.cpp#L240-L248" target="_blank">source</a>]








### RmlUi.ClearTranslations
---
```lua
function RmlUi.ClearTranslations()
```





Clear registered translations.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Global.cpp#L252-L256" target="_blank">source</a>]








### RmlUi.SetMouseCursorAlias
---
```lua
function RmlUi.SetMouseCursorAlias(
  rml_name: string,
  recoil_name: string
)
```
@param `rml_name` - name used in rml script

@param `recoil_name` - name used in recoil






Converts the css names for cursors to the Recoil Engine names for cursors like `RmlUi.SetMouseCursorAlias("pointer", 'Move')`.
Web devs tend to want to use specific words for pointer types.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Global.cpp#L260-L268" target="_blank">source</a>]








### RmlUi.SetDebugContext
---
```lua
function RmlUi.SetDebugContext(context: (string|RmlUi.Context))
```





Set which context the debug inspector is meant to inspect.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Global.cpp#L270-L276" target="_blank">source</a>]











## fields
---

### RmlUi.RmlModalFlag
---
```lua
RmlUi.RmlModalFlag : RmlUi.RmlModalFlag {
    None: integer,
    Modal: integer,
    Keep: integer,
}
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Document.cpp#L102-L104" target="_blank">source</a>]








### RmlUi.RmlFocusFlag
---
```lua
RmlUi.RmlFocusFlag : RmlUi.RmlFocusFlag {
    None: integer,
    Document: integer,
    Keep: integer,
    Auto: integer,
}
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Document.cpp#L116-L118" target="_blank">source</a>]








### RmlUi.Document
---
```lua
RmlUi.Document : RmlUi.Document {
    title: string,
    context: RmlUi.Context,
    url: string,
    modal: boolean,
    widget: table,
}
```



Document derives from Element. Document has no constructor; it must be instantiated through a Context object instead, either by loading an external RML file or creating an empty document. It has the following functions and properties:

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Document.cpp#L132-L135" target="_blank">source</a>]








### RmlUi.Vector2i
---
```lua
RmlUi.Vector2i : RmlUi.Vector2i {
    x: integer,
    y: integer,
    magnitude: number,
}
```



Two-dimensional integral vector.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Vector.cpp#L40-L48" target="_blank">source</a>]








### RmlUi.Vector2f
---
```lua
RmlUi.Vector2f : RmlUi.Vector2f {
    x: number,
    y: number,
    magnitude: number,
}
```



Two-dimensional float vector

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Vector.cpp#L83-L92" target="_blank">source</a>]





@see float2




### RmlUi.ElementPtr
---
```lua
RmlUi.ElementPtr : RmlUi.ElementPtr
```



Represents an owned element. This type is mainly used to modify the DOM tree by passing the object into other elements. For example `RmlUi.Element:AppendChild()`.
A current limitation in the Lua plugin is that Element member properties and functions cannot be used directly on this type.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Global.cpp#L105-L109" target="_blank">source</a>]








### RmlUi.contexts
---
```lua
RmlUi.contexts : RmlUi.Context[]
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Global.cpp#L280-L280" target="_blank">source</a>]








### RmlUi.version
---
```lua
RmlUi.version : string
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Global.cpp#L283-L283" target="_blank">source</a>]
RmlUi version








### RmlUi.key_identifier
---
```lua
RmlUi.key_identifier : RmlUi.key_identifier {
    0: integer,
    1: integer,
    2: integer,
    3: integer,
    4: integer,
    5: integer,
    6: integer,
    7: integer,
    8: integer,
    9: integer,
    UNKNOWN: integer,
    SPACE: integer,
    ...
}
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Global.cpp#L286-L298" target="_blank">source</a>]








### RmlUi.key_modifier
---
```lua
RmlUi.key_modifier : RmlUi.key_modifier {
    CTRL: integer,
    SHIFT: integer,
    ALT: integer,
    META: integer,
    CAPSLOCK: integer,
    NUMLOCK: integer,
    SCROLLLOCK: integer,
}
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Global.cpp#L648-L650" target="_blank">source</a>]








### RmlUi.font_weight
---
```lua
RmlUi.font_weight : RmlUi.font_weight {
    Auto: integer,
    Normal: integer,
    Bold: integer,
}
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Global.cpp#L670-L672" target="_blank">source</a>]








### RmlUi.default_action_phase
---
```lua
RmlUi.default_action_phase : RmlUi.default_action_phase {
    Auto: integer,
    Target: integer,
    TargetAndBubble: integer,
}
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Global.cpp#L684-L686" target="_blank">source</a>]








### RmlUi.ElementText
---
```lua
RmlUi.ElementText : RmlUi.ElementText {
    text: string,
}
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/ElementDerived.cpp#L44-L46" target="_blank">source</a>]








### RmlUi.ElementTabSet
---
```lua
RmlUi.ElementTabSet : RmlUi.ElementTabSet {
    active_tab: integer,
    num_tabs: integer,
}
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/ElementDerived.cpp#L57-L59" target="_blank">source</a>]








### RmlUi.ElementProgress
---
```lua
RmlUi.ElementProgress : RmlUi.ElementProgress {
    value: number,
    max: number,
}
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/ElementDerived.cpp#L99-L101" target="_blank">source</a>]








### RmlUi.RmlEventPhase
---
```lua
RmlUi.RmlEventPhase : RmlUi.RmlEventPhase {
    None: integer,
    Capture: integer,
    Target: integer,
    Bubble: integer,
}
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Event.cpp#L66-L68" target="_blank">source</a>]








### RmlUi.EventParametersProxy
---
```lua
RmlUi.EventParametersProxy : RmlUi.EventParametersProxy {
    button: RmlUi.EventParametersProxy.MouseButton,
    mouse_x: integer,
    mouse_y: integer,
    scroll_lock_key: RmlUi.EventParametersProxy.TrueFalse,
    meta_key: RmlUi.EventParametersProxy.TrueFalse,
    ctrl_key: RmlUi.EventParametersProxy.TrueFalse,
    shift_key: RmlUi.EventParametersProxy.TrueFalse,
    caps_lock_key: RmlUi.EventParametersProxy.TrueFalse,
    alt_key: RmlUi.EventParametersProxy.TrueFalse,
}
```



Keyboard and mouse input data associated with an event.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Event.cpp#L94-L106" target="_blank">source</a>]








### RmlUi.Event
---
```lua
RmlUi.Event : RmlUi.Event {
    current_element: RmlUi.Element,
    type: string,
    target_element: RmlUi.Element,
    parameters: RmlUi.EventParametersProxy,
    event_phase: RmlUi.RmlEventPhase,
    interruptible: boolean,
    propagating: boolean,
    immediate_propagating: boolean,
}
```



An event that happens to an element.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Event.cpp#L108-L111" target="_blank">source</a>]








### RmlUi.EventListener
---
```lua
RmlUi.EventListener : RmlUi.EventListener
```



Event listener interface

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L198-L201" target="_blank">source</a>]








### RmlUi.Element
---
```lua
RmlUi.Element : RmlUi.Element {
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



Represents an element in the RmlUi document tree. This class cannot be constructed directly; use a Document object to instantiate elements. This is the foundational piece of the DOM.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L231-L234" target="_blank">source</a>]








### RmlUi.ElementForm
---
```lua
RmlUi.ElementForm : RmlUi.ElementForm
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/ElementForm.cpp#L139-L141" target="_blank">source</a>]








### RmlUi.ElementFormControl
---
```lua
RmlUi.ElementFormControl : RmlUi.ElementFormControl {
    disabled: boolean,
    name: string,
    value: string,
    submitted: boolean,
}
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/ElementForm.cpp#L156-L158" target="_blank">source</a>]








### RmlUi.ElementFormControlInput
---
```lua
RmlUi.ElementFormControlInput : RmlUi.ElementFormControlInput {
    checked: boolean,
    maxlength: integer,
    size: integer,
    max: integer,
    min: integer,
    step: integer,
}
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/ElementForm.cpp#L178-L180" target="_blank">source</a>]








### RmlUi.ElementFormControlSelect
---
```lua
RmlUi.ElementFormControlSelect : RmlUi.ElementFormControlSelect
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/ElementForm.cpp#L216-L218" target="_blank">source</a>]








### RmlUi.ElementFormControlTextArea
---
```lua
RmlUi.ElementFormControlTextArea : RmlUi.ElementFormControlTextArea {
    cols: integer,
    maxlength: integer,
    rows: integer,
    wordwrap: boolean,
}
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/ElementForm.cpp#L254-L256" target="_blank">source</a>]








### RmlUi.SolLuaDataModel
---
```lua
RmlUi.SolLuaDataModel : RmlUi.SolLuaDataModel<T>
```



Handle for your data model. It is a wrapper around the model table, marked as type T.
You can access fields by using normal indexing, but for the moment, the only keys that work are strings. Any index done this way will automatically trigger a rerender.
If you need to index any tables or subtables by anything not a string, you will need to use the underlying table, gotten with `__GetTable`. This will not trigger a rerender.
To trigger a rerender manually, use `_SetDirty`, passing in the name of the top-level entry in your model table that you edited.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/DataModel.cpp#L60-L66" target="_blank">source</a>]








### RmlUi.Context
---
```lua
RmlUi.Context : RmlUi.Context {
    dimensions: RmlUi.Vector2i,
    dp_ratio: number,
    documents: RmlUi.Document[],
    focus_element: RmlUi.Element,
    hover_element: RmlUi.Element,
    name: string,
    root_element: RmlUi.Element,
}
```



Holds documents and a data model.
The Context class has no constructor; it must be instantiated through the CreateContext() function. It has the following functions and properties:

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Context.cpp#L283-L287" target="_blank">source</a>]










{% endraw %}