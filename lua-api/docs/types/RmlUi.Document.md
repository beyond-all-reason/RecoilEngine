---
layout: default
title: Document
parent: Lua API
permalink: lua-api/types/Document
---

{% raw %}

# class Document


- supers: RmlUi.Element


- namespace: RmlUi



Document derives from Element. Document has no constructor; it must be instantiated through a Context object instead, either by loading an external RML file or creating an empty document. It has the following functions and properties:

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Document.cpp#L132-L135" target="_blank">source</a>]







---

## methods
---

### Document.PullToFront
---
```lua
function Document.PullToFront()
```





Pulls the document in front of other documents within its context with a similar z-index.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Document.cpp#L139-L142" target="_blank">source</a>]








### Document.PushToBack
---
```lua
function Document.PushToBack()
```





Pushes the document behind other documents within its context with a similar z-index.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Document.cpp#L144-L147" target="_blank">source</a>]








### Document.Show
---
```lua
function Document.Show(
  modal: RmlUi.RmlModalFlag?,
  focus: RmlUi.RmlFocusFlag?
)
```
@param `modal` - Defaults to Focus






Shows the document.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Document.cpp#L149-L154" target="_blank">source</a>]








### Document.Hide
---
```lua
function Document.Hide()
```





Hides the document.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Document.cpp#L156-L159" target="_blank">source</a>]








### Document.Close
---
```lua
function Document.Close()
```





Hides and closes the document, destroying its contents.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Document.cpp#L161-L164" target="_blank">source</a>]








### Document.CreateElement
---
```lua
function Document.CreateElement(tag_name: string) ->  RmlUi.ElementPtr
```





Instances an element with a tag of tag_name.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Document.cpp#L166-L171" target="_blank">source</a>]








### Document.CreateTextNode
---
```lua
function Document.CreateTextNode(text: string) ->  RmlUi.ElementPtr
```





Instances a text element containing the string text.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Document.cpp#L173-L178" target="_blank">source</a>]








### Document.ReloadStyleSheet
---
```lua
function Document.ReloadStyleSheet(load: boolean?)
```





Reload the active style sheet.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Document.cpp#L181-L185" target="_blank">source</a>]








### Document.LoadInlineScript
---
```lua
function Document.LoadInlineScript(
  content: string,
  source: string?,
  source_line: integer?
)
```





Load scripts as if it were in the script tag.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Document.cpp#L187-L193" target="_blank">source</a>]








### Document.LoadExternalScript
---
```lua
function Document.LoadExternalScript(source_path: string)
```





Load an external script.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Document.cpp#L195-L199" target="_blank">source</a>]








### Document.UpdateDocument
---
```lua
function Document.UpdateDocument()
```





Update the Document.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Document.cpp#L201-L204" target="_blank">source</a>]








### Document.AppendToStyleSheet
---
```lua
function Document.AppendToStyleSheet(content: string)
```





Append text to style sheet.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Document.cpp#L206-L210" target="_blank">source</a>]











## fields
---

### Document.title
---
```lua
Document.title : string
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Document.cpp#L214-L214" target="_blank">source</a>]








### Document.context
---
```lua
Document.context : RmlUi.Context {
    dimensions: RmlUi.Vector2i,
    dp_ratio: number,
    documents: RmlUi.Document[],
    focus_element: RmlUi.Element,
    hover_element: RmlUi.Element,
    name: string,
    root_element: RmlUi.Element,
}
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Document.cpp#L218-L218" target="_blank">source</a>]








### Document.url
---
```lua
Document.url : string
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Document.cpp#L221-L221" target="_blank">source</a>]








### Document.modal
---
```lua
Document.modal : boolean
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Document.cpp#L223-L223" target="_blank">source</a>]
Is it modal?








### Document.widget
---
```lua
Document.widget: table
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Document.cpp#L225-L225" target="_blank">source</a>]
A table of data that can be accessed in onevent attributes. It doesn't have to be a widget.










{% endraw %}