---
layout: default
title: Encoding
parent: Lua API
permalink: lua-api/globals/Encoding
---

{% raw %}

# global Encoding


Lua Encoding API

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaEncoding.cpp#L22-L25" target="_blank">source</a>]







---

## methods
---

### Encoding.DecodeBase64
---
```lua
function Encoding.DecodeBase64(text: string) -> decoded string
```
@param `text` - Text to decode


@return `decoded` - Decoded text





Decodes a base64 string

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaEncoding.cpp#L41-L50" target="_blank">source</a>]

Decodes a base64 encoded string. If it encounters
invalid data, it stops and returns whatever it parsed so far.








### Encoding.EncodeBase64
---
```lua
function Encoding.EncodeBase64(
  text: string,
  stripPadding: boolean?
) -> encoded string
```
@param `text` - Text to encode

@param `stripPadding` - Remove padding (`=` characters) at the end when 'true'. Defaults to `true`.


@return `encoded` - Encoded text





Encodes a base64 string

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaEncoding.cpp#L61-L68" target="_blank">source</a>]








### Encoding.IsValidBase64
---
```lua
function Encoding.IsValidBase64(text: string) -> valid boolean
```
@param `text` - Text to validate


@return `valid` - Whether the text is valid base64





Validates a base64 string

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaEncoding.cpp#L85-L91" target="_blank">source</a>]








### Encoding.DecodeBase64Url
---
```lua
function Encoding.DecodeBase64Url(text: string) -> decoded string
```
@param `text` - Text to decode


@return `decoded` - Decoded text





Decodes a base64url string

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaEncoding.cpp#L109-L118" target="_blank">source</a>]

Decodes a base64url encoded string. If it encounters
invalid data, it stops and returns whatever it parsed so far.








### Encoding.EncodeBase64Url
---
```lua
function Encoding.EncodeBase64Url(text: string) -> encoded string
```
@param `text` - Text to encode


@return `encoded` - Encoded text





Encodes a base64url string

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaEncoding.cpp#L136-L142" target="_blank">source</a>]








### Encoding.IsValidBase64Url
---
```lua
function Encoding.IsValidBase64Url(text: string) -> valid boolean
```
@param `text` - Text to validate


@return `valid` - Whether the text is valid base64url





Validates a base64url string

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaEncoding.cpp#L164-L170" target="_blank">source</a>]












{% endraw %}