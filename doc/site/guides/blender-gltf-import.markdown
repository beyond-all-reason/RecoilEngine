---
layout: post
title: How to import a GLTF file from Blender
parent: Guides
permalink: guides/blender-gltf-import
author: lhog
---
# Intro
## About the article
The article provides generic information to use GLTF models instead or in addition to the supported Assimp(Collada only, practically speaking) and s3o models. In the articles we don't touch the subjects of modeling, texturing, skinning or animation. Only the technicallities of Blender GLTF exporter and Recoil importer of GLTF.
The guide is written in a prescriptive way. Try to not deviate from the guide too much, otherwise you may end up in the unsupported territory.

## About GLTF 2.0
The engine has recently instroduced the support to [GLTF 2.0](https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html) model file format. It's also widely supported throughout the industry.

## Why use GLTF in Recoil?
- Widely supported unlike s3o
- Modern and in-trend unlike Collada
- Supports skinning, which Recoil supports. s3o will never have that.
- Supports the embedded animation (future support in Recoil). s3o will never have that either.
- Should load faster than Collada
- The supporting library is self-contained, unlike monstrous Assimp, which we can't update due to sync concerns

## GLTF support in Recoil
- The engine versions after [this commit](https://github.com/beyond-all-reason/RecoilEngine/commit/d20e7f15024db58ade60dff3dbcb9590c94a316a) should have working experimental GLTF support (could still be bugged).
- The only supported primitive type is `Triangles`. `Points`, `Lines`, `LineLoops`, `LineStrips`, `TriangleStrips` and `TriangleFans` are not supported. The last two might be added on demand (and for good reason). The unsupported primitives will cause runtime fatal error
- Skinning is supposed to be supported, but was never tested after initial implementation
- Embedded animation is not supported, but maybe will be supported in the future versions of the engine
- Recoil ignores textures and materials, mostly because they're expected to follow the PBR workflow
- Other vertex attributes, e.g. vertex colors, are not loaded, but can be considered for addition in the future versions of the engine

## Side notes
- [S3O Kit](https://github.com/ChrisFloofyKitsune/s3o-blender-tools) provides a great Blender plugin to import existing s3o models from Blender. The s3o Kit imports many s3o attributes as separate nodes, you might want to remove them from the model, before exporting it to GLTF2, because they cause creation of extra meaningless pieces. Make sure the model hierarchy is clean of s3o attributes leftovers.


# Export from Blender
## Important things to know
Your model inside Blender SHOULD be positioned Z-up. Effects of other positioning are not explored, most likely it just won't work.
![image]({{ site.baseurl }}/assets/guides/blender-gltf-1.png)

Before you export the model it might be convinient to define the **Scene**'s custom properties that are respected by the Recoil loader
![image]({{ site.baseurl }}/assets/guides/blender-gltf-1b.png)

As the properties are custom, you will need to define them for each new model. If better ways are known, feel free to add this to the guide.
![image]({{ site.baseurl }}/assets/guides/blender-gltf-1a.png)

The list of supported key-values is following:
- `tex1` - string - relative path to `tex1` in `UnitTextures` directory, empty by default
- `tex2` - string - relative path to `tex2` in `UnitTextures` directory, empty by default
- `midpos` - float array, len 3 - relative middle of the model position, by default calculated based on the model dimensions
- `mins` - float array, len 3 - minimum bounds of the model, in the model space, by default calculated based on the model dimensions
- `maxs` - float array, len 3 - maximum bounds of the model, in the model space, by default calculated based on the model dimensions
- `height` - float - the height of the model, by default calculated based on the model dimensions
- `radius` - float - the radius of the model, by default calculated based on the model dimensions
- `fliptextures` - boolean - whether to flip the supplied tex1 / tex2 (if supported by the texture types). False by default
- `invertteamcolor` - boolean - whether to inverse the teamcolor in tex1 (if supported by the texture types). False by default
- `s3ocompat` - boolean - transform the model such that its animation and worldspace position matches those of s3o (left hand coordinate system). False by default. This option is even more experimental than the loader as whole.

Alternatively all the same keys can be defined in a Lua file, right next to the model, the same as it's done for Collada / `.dae`. Note that only the options above are supported for GLTF format. All the rest Assimp Lua keys and tables are ignored.
If both the custom Scene attributes in the GLTF file and the Lua file are present, the values in the Lua file will take precedence.

## Export step list
1. Go to  File --> Export --> glTF 2.0
![image]({{ site.baseurl }}/assets/guides/blender-gltf-2.png)
2. Select `binary glTF format (.glb)` as the file format. Make sure to include `Custom properties` if you chose to define them.
*`.gltf` (text) format is nice for debugging, the engine might load it, but it's never been tested any thoroughly*
![image]({{ site.baseurl }}/assets/guides/blender-gltf-3.png)
3. You MUST make sure to unclick `+Y up`
![image]({{ site.baseurl }}/assets/guides/blender-gltf-4.png)
4. Make sure to export `UVs`, `Normals`, `Tangents`.
![image]({{ site.baseurl }}/assets/guides/blender-gltf-5.png)
5. The rest of the settings are listed on the screenshots below
![image]({{ site.baseurl }}/assets/guides/blender-gltf-6.png)
![image]({{ site.baseurl }}/assets/guides/blender-gltf-7.png)
6. Finally choose the folder and filename and press `Export glTF 2.0` button
![image]({{ site.baseurl }}/assets/guides/blender-gltf-8.png)

*You can export additional attributes not listed on the screenshots, there's no harm in that*

# Load the GLTF model in Recoil
Out of all custom attributes defined in the Scene's GLTF or in the Lua file the two you should probably define in any case are `tex1` and `tex2`. The rest are defined per use case.
In order to load the GLTF model, make sure it resides somewhere in `Objects3d` directory of your game. Next in the Unit / Feature / WeaponDef just reference the model's relative file with `.glb` extension.

## S3O Compat
If you export your existing s3o model as GLTF and reuse the existing animation, made for s3o model and you see your units are walking "backwards", make sure to instruct the importer to try to rotate the model in s3o compatible way (define `s3ocompat` to `true`).
By default, GLTF is imported in the same (right handed) coordinate system as Collada / `.dae` and thus require changes to the animation scripts.
![image]({{ site.baseurl }}/assets/guides/blender-gltf-9.png)

## Nodes hierarchy
The importer will create a few empty top level pieces to accomodate for rotations and other nuances of the importer. This is usually completely transparent to the game devs. This might be optimized later on.