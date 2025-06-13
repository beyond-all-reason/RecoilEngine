---
layout: default
title: Element
parent: Lua API
permalink: lua-api/types/Element
---

{% raw %}

# class Element



- namespace: RmlUi



Represents an element in the RmlUi document tree. This class cannot be constructed directly; use a Document object to instantiate elements. This is the foundational piece of the DOM.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L231-L234" target="_blank">source</a>]







---

## methods
---

### Element.AddEventListener
---
```lua
function Element.AddEventListener(
  event: string,
  listener: (function|string),
  in_capture_phase: boolean
)
```





Adds an event listener to the element.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L240-L246" target="_blank">source</a>]








### Element.AppendChild
---
```lua
function Element.AppendChild(element: RmlUi.ElementPtr) ->  RmlUi.ElementPtr
```





Appends element as a child to this element.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L253-L258" target="_blank">source</a>]








### Element.Blur
---
```lua
function Element.Blur()
```





Removes input focus from this element.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L262-L265" target="_blank">source</a>]








### Element.Click
---
```lua
function Element.Click()
```





Fakes a click on this element.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L267-L270" target="_blank">source</a>]








### Element.DispatchEvent
---
```lua
function Element.DispatchEvent(
  event: string,
  parameters: table,
  interruptible: string
) ->  boolean
```





Dispatches an event to this element.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L272-L279" target="_blank">source</a>]








### Element.Focus
---
```lua
function Element.Focus()
```





Gives input focus to this element.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L281-L284" target="_blank">source</a>]








### Element.GetAttribute
---
```lua
function Element.GetAttribute(name: string) ->  any
```





Returns the value of the attribute named name. If no such attribute exists, the empty string will be returned.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L286-L291" target="_blank">source</a>]








### Element.GetElementById
---
```lua
function Element.GetElementById(id: string) ->  RmlUi.Element
```





Returns the descendant element with an id of id.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L293-L298" target="_blank">source</a>]








### Element.GetElementsByTagName
---
```lua
function Element.GetElementsByTagName(tag_name: string) ->  RmlUi.ElementPtr[]
```





Returns a list of all descendant elements with the tag of tag_name.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L300-L305" target="_blank">source</a>]








### Element.QuerySelector
---
```lua
function Element.QuerySelector(query: string) ->  RmlUi.ElementPtr
```





Unsure what this does, but seems powerful?

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L307-L312" target="_blank">source</a>]








### Element.QuerySelectorAll
---
```lua
function Element.QuerySelectorAll(selectors: string)
```





Unsure what this does, but seems powerful?

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L314-L318" target="_blank">source</a>]








### Element.HasAttribute
---
```lua
function Element.HasAttribute(name: string) ->  boolean
```





Returns True if the element has a value for the attribute named name, False if not.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L320-L325" target="_blank">source</a>]








### Element.HasChildNodes
---
```lua
function Element.HasChildNodes() ->  boolean
```





Returns True if the element has at least one child node, false if not.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L327-L331" target="_blank">source</a>]








### Element.InsertBefore
---
```lua
function Element.InsertBefore(
  element: RmlUi.ElementPtr,
  adjacent_element: RmlUi.Element
) ->  RmlUi.Element?
```





Inserts the element element as a child of this element, directly before adjacent_element in the list of children.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L333-L339" target="_blank">source</a>]








### Element.IsClassSet
---
```lua
function Element.IsClassSet(name: string) ->  boolean
```





Returns true if the class name is set on the element, false if not.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L343-L348" target="_blank">source</a>]








### Element.RemoveAttribute
---
```lua
function Element.RemoveAttribute(name: string)
```





Removes the attribute named name from the element.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L350-L354" target="_blank">source</a>]








### Element.RemoveChild
---
```lua
function Element.RemoveChild(element: RmlUi.Element) ->  boolean
```





Removes the child element element from this element.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L356-L361" target="_blank">source</a>]








### Element.ReplaceChild
---
```lua
function Element.ReplaceChild(
  inserted_element: RmlUi.ElementPtr,
  replaced_element: RmlUi.Element
) ->  boolean
```





Replaces the child element replaced_element with inserted_element in this element's list of children. If replaced_element is not a child of this element, inserted_element will be appended onto the list instead.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L363-L369" target="_blank">source</a>]








### Element.ScrollIntoView
---
```lua
function Element.ScrollIntoView(align_with_top: boolean)
```





Scrolls this element into view if its ancestors have hidden overflow.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L373-L377" target="_blank">source</a>]








### Element.SetAttribute
---
```lua
function Element.SetAttribute(
  name: string,
  value: string
)
```





Sets the value of the attribute named name to value.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L384-L389" target="_blank">source</a>]








### Element.SetClass
---
```lua
function Element.SetClass(
  name: string,
  value: boolean
)
```





Sets (if value is true) or clears (if value is false) the class name on the element.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L391-L396" target="_blank">source</a>]








### Element.GetElementsByClassName
---
```lua
function Element.GetElementsByClassName(class_name: string) ->  RmlUi.Element[]
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L399-L403" target="_blank">source</a>]








### Element.Clone
---
```lua
function Element.Clone() ->  RmlUi.ElementPtr
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L405-L408" target="_blank">source</a>]








### Element.Closest
---
```lua
function Element.Closest() ->  RmlUi.Element?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L410-L413" target="_blank">source</a>]








### Element.SetPseudoClass
---
```lua
function Element.SetPseudoClass(class_name: string)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L415-L418" target="_blank">source</a>]








### Element.IsPseudoClassSet
---
```lua
function Element.IsPseudoClassSet(class_name: string) ->  boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L420-L424" target="_blank">source</a>]








### Element.ArePseudoCLassesSet
---
```lua
function Element.ArePseudoCLassesSet(class_names: string[]) ->  boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L426-L430" target="_blank">source</a>]








### Element.GetActivePseudoCLasses
---
```lua
function Element.GetActivePseudoCLasses() ->  string[]
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L432-L435" target="_blank">source</a>]








### Element.IsPointWithinElement
---
```lua
function Element.IsPointWithinElement(point: RmlUi.Vector2i) ->  boolean
```





Is a screen-space point within this element?

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L437-L442" target="_blank">source</a>]








### Element.ProcessDefaultAction
---
```lua
function Element.ProcessDefaultAction(event: RmlUi.Event)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L444-L447" target="_blank">source</a>]








### Element.GetValue
---
```lua
function Element.GetValue() -> value (number|string|"")
```

@return `value` - Returns number if it has the tag "input", a string if it has the tag "textarea", else an empty string.





Get the value of this element.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L449-L453" target="_blank">source</a>]








### Element.GetChild
---
```lua
function Element.GetChild(index: integer) ->  RmlUi.Element?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L462-L466" target="_blank">source</a>]











## fields
---

### Element.class_name
---
```lua
Element.class_name : string
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L470-L470" target="_blank">source</a>]
Name of the class.








### Element.id
---
```lua
Element.id : string
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L472-L472" target="_blank">source</a>]
ID of this element, in the context of `<span id="foo">`.








### Element.inner_rml
---
```lua
Element.inner_rml : string
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L474-L474" target="_blank">source</a>]
Gets or sets the inner RML (markup) content of the element.








### Element.scroll_left
---
```lua
Element.scroll_left : integer
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L476-L476" target="_blank">source</a>]
Gets or sets the number of pixels that the content of the element is scrolled from the left.








### Element.scroll_top
---
```lua
Element.scroll_top : integer
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L478-L478" target="_blank">source</a>]
Gets or sets the number of pixels that the content of the element is scrolled from the top.








### Element.attributes
---
```lua
Element.attributes : RmlUi.ElementAttributesProxy
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L482-L482" target="_blank">source</a>]
Read-only. Proxy for accessing element attributes.








### Element.child_nodes
---
```lua
Element.child_nodes : RmlUi.ElementChildNodesProxy
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L484-L484" target="_blank">source</a>]
Read-only. Proxy for accessing child nodes of the element.








### Element.client_left
---
```lua
Element.client_left : integer
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L486-L486" target="_blank">source</a>]
Read-only. The width of the left border of the element in pixels.








### Element.client_height
---
```lua
Element.client_height : integer
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L488-L488" target="_blank">source</a>]
Read-only. The inner height of the element in pixels, including padding but not the horizontal scrollbar height, border, or margin.








### Element.client_top
---
```lua
Element.client_top : integer
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L490-L490" target="_blank">source</a>]
Read-only. The width of the top border of the element in pixels.








### Element.client_width
---
```lua
Element.client_width : integer
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L492-L492" target="_blank">source</a>]
Read-only. The inner width of the element in pixels, including padding but not the vertical scrollbar width, border, or margin.








### Element.first_child
---
```lua
Element.first_child : RmlUi.Element?
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L494-L494" target="_blank">source</a>]
Read-only. The first child element, or nil if there are no children.








### Element.last_child
---
```lua
Element.last_child : RmlUi.Element?
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L496-L496" target="_blank">source</a>]
Read-only. The last child element, or nil if there are no children.








### Element.next_sibling
---
```lua
Element.next_sibling : RmlUi.Element?
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L498-L498" target="_blank">source</a>]
Read-only. The next sibling element, or nil if there is none.








### Element.offset_height
---
```lua
Element.offset_height : integer
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L500-L500" target="_blank">source</a>]
Read-only. The height of the element including vertical padding and borders, in pixels.








### Element.offset_left
---
```lua
Element.offset_left : integer
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L502-L502" target="_blank">source</a>]
Read-only. The distance from the inner left edge of the offset parent, in pixels.








### Element.offset_parent
---
```lua
Element.offset_parent : RmlUi.Element {
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



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L504-L504" target="_blank">source</a>]
Read-only. The closest positioned ancestor element.








### Element.offset_top
---
```lua
Element.offset_top : integer
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L506-L506" target="_blank">source</a>]
Read-only. The distance from the inner top edge of the offset parent, in pixels.








### Element.offset_width
---
```lua
Element.offset_width : integer
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L508-L508" target="_blank">source</a>]
Read-only. The width of the element including horizontal padding and borders, in pixels.








### Element.owner_document
---
```lua
Element.owner_document : RmlUi.Document {
    title: string,
    context: RmlUi.Context,
    url: string,
    modal: boolean,
    widget: table,
}
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L510-L510" target="_blank">source</a>]
Read-only. The document that owns this element.








### Element.parent_node
---
```lua
Element.parent_node : RmlUi.Element?
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L512-L512" target="_blank">source</a>]
Read-only. The parent node of this element, or nil if there is none.








### Element.previous_sibling
---
```lua
Element.previous_sibling : RmlUi.Element?
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L514-L514" target="_blank">source</a>]
Read-only. The previous sibling element, or nil if there is none.








### Element.scroll_height
---
```lua
Element.scroll_height : integer
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L516-L516" target="_blank">source</a>]
Read-only. The total height of the element's content, including content not visible on the screen due to overflow.








### Element.scroll_width
---
```lua
Element.scroll_width : integer
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L518-L518" target="_blank">source</a>]
Read-only. The total width of the element's content, including content not visible on the screen due to overflow.








### Element.style
---
```lua
Element.style : RmlUi.ElementStyleProxy
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L520-L520" target="_blank">source</a>]
Read-only. Proxy for accessing and modifying the element's style properties.








### Element.tag_name
---
```lua
Element.tag_name : string
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L522-L522" target="_blank">source</a>]
Read-only. The tag name of the element.








### Element.address
---
```lua
Element.address : string
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L525-L525" target="_blank">source</a>]
Read-only. The address of the element in the document tree.








### Element.absolute_left
---
```lua
Element.absolute_left : integer
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L527-L527" target="_blank">source</a>]
Read-only. The absolute left position of the element relative to the document.








### Element.absolute_top
---
```lua
Element.absolute_top : integer
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L529-L529" target="_blank">source</a>]
Read-only. The absolute top position of the element relative to the document.








### Element.baseline
---
```lua
Element.baseline : integer
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L531-L531" target="_blank">source</a>]
Read-only. The baseline position of the element.








### Element.line_height
---
```lua
Element.line_height : integer
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L533-L533" target="_blank">source</a>]
Read-only. The computed line height of the element.








### Element.visible
---
```lua
Element.visible : boolean
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L535-L535" target="_blank">source</a>]
Read-only. True if the element is visible, false otherwise.








### Element.z_index
---
```lua
Element.z_index : integer
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L537-L537" target="_blank">source</a>]
Read-only. The computed z-index of the element.










{% endraw %}