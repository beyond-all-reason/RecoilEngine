---
title: Texture atlas manifests
---

# Texture atlas manifests

Recoil can load immutable, game-authored texture atlases from
`gamedata/textureatlases.lua`. If that file is absent, all legacy procedural
atlas behavior is unchanged. Fonts, 3DO textures, and BumpWater are outside this
system.

The registry is a data-only Lua table keyed by atlas ID. IDs are unique under
ASCII case folding. Each definition requires `manifest`, may list ordered
`patches`, and may claim one of the unique roles `explosions`, `groundfx`,
`decals`, or `icons0`.

```lua
return {
  example = {
    manifest = "bitmaps/atlases/example.atlas.lua",
    patches = { "bitmaps/atlases/example.patch.lua" },
    role = "groundfx",
  },
}
```

## Manifest v2

A manifest returns a table with schema `recoil.texture-atlas`, version `2`, a
`2d` target, positive dimensions and mip count, `coordinateOrigin = "top-left"`,
ordered DDS variants, pages, default padding, and entries. Rectangles are
integer, top-left-origin, half-open pixel rectangles. `content` is sampled;
`allocation` is the collision rectangle and contains authored padding. UVs are
derived from `content` and are never authoritative input.

Every variant supplies exactly one DDS file per page. All files have identical
dimensions and mip counts. Runtime profiles are BC1, BC2, BC3, BC4, BC5, BC7,
and RGBA8 where the renderer supports them. Files authored through this path are
uploaded without extension-dependent flipping.

Entry IDs are lookup keys and are ASCII-case-folded. `source` retains exact VFS
spelling. Duplicate case variants are fatal. Entries and reserves may not
overlap or leave their page. Unused packing gaps are not patch capacity.

Clamp padding adds `pixels` on all sides and copies edge pixels. Tile padding
uses `tilesX` and `tilesY`; allocation size is `(2*tilesX+1)*W` by
`(2*tilesY+1)*H`, and the content is the central complete tile.

## Patch sidecars

A sidecar uses schema `recoil.texture-atlas-patch`, version `1`, and maps entry
IDs to replacement source paths plus optional padding. Duplicate IDs across the
complete ordered sidecar set are fatal. Existing entries reuse a fitting fixed
allocation. Larger replacements and new entries use only declared reserves.
Superseded allocations are not recycled in the startup batch. The allocator is
deterministic, disables rotation, and commits nothing unless every request fits.

Missing sources, invalid images, collisions, or reserve exhaustion are content
errors. After planning, metadata is stable. After GPU finalization, the atlas is
immutable; texture reload recreates the committed contents without discovering
new patches or changing layout.

Untouched atlases retain their selected DDS compression. A patched atlas is
rebuilt as RGBA8 and logs a recommendation to rebuild it offline. The
`TextureAtlasVariant` setting defaults to `auto`; a forced format must exist and
be supported for every page.

Headless builds parse and validate the registry, manifests, sidecars, geometry,
and DDS headers without pretending to have GPU format support.

## Debugging and compatibility

Engine-owned atlases are referenced as `$atlas:<id>`. Legacy role strings remain
valid. The v2 Lua metadata API reports dimensions, pages, mip levels, selected
variant, modification/compression state, finalized UVs, page, and pixel size.
Lua-owned mutable atlas APIs remain separate.

Debug dumps write one TGA per page and a valid v2 manifest describing finalized
pixel rectangles. Debug output is RGBA8 and is not a production-authorship
artifact.
