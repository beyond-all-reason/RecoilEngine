# Polygon collision volumes

A fork of [RecoilEngine](https://github.com/beyond-all-reason/RecoilEngine)
adding a per-piece collision volume type that traces a piece's real
triangles instead of fitting an axis-aligned box around them.

*Русская версия — ниже. / Russian version below.*

---

## English

### The problem

Recoil builds each piece's default collision volume as an axis-aligned
bounding box over that piece's vertices — see `ModelUtils.cpp`,
`S3OParser.cpp` and `3DOParser.cpp`, all of which do:

```cpp
piece->SetCollisionVolume(CollisionVolume('b', 'z',
    piece->maxs - piece->mins,
    (piece->maxs + piece->mins) * 0.5f));
```

For a piece that sits at an angle — sloped armour, angled plating, a
rotated module — that box is considerably larger than the piece itself
and protrudes into empty space. Shots register against nothing.

Collision volumes have no rotation parameter, so the box cannot be
tilted to match. The only workaround available today is rotating the
piece itself from a script, which also rotates the visible mesh and is
impractical when many differently-angled parts are involved.

### The approach

Rather than adding a second source of orientation, this fork adds a
volume type whose shape *is* the geometry: `COLVOL_TYPE_POLYGON`.

Orientation comes from the piece, through the same
`LocalModelPiece::GetModelSpaceMatrix()` path every other volume type
uses. The piece's vertices already live in piece-local space
(`ModelUtils.cpp`: *"transform model space mesh vertices into bone/piece
space"*), which is exactly the space `CCollisionHandler::Intersect()`
works in, so no extra transform is introduced anywhere.

### Usage

In the UnitDef:

```lua
collisionVolumeType = "polygon",
```

That is all. Every piece of the unit switches to polygon volumes and
piece-tree hit testing is enabled automatically — `usePieceCollisionVolumes`
is implied and does not need to be written.

For **mixed** setups, leave `collisionVolumeType` out, set

```lua
usePieceCollisionVolumes = true,
```

and assign volumes per piece from the unit script or a gadget:

```lua
-- volume types: 0 ellipsoid, 1 cylinder, 2 box, 3 sphere, 4 polygon
Spring.SetUnitPieceCollisionVolumeData(unitID, piece("hull"), true,
    0,0,0, 0,0,0, 4, 0)       -- polygon: scales and offsets unused

Spring.SetUnitPieceCollisionVolumeData(unitID, piece("turret"), true,
    30,20,30, 0,10,0, 2, 0)   -- box: scales and offsets do apply
```

Excluding a piece from hit detection works as with any volume type — via
the third argument:

```lua
Spring.SetUnitPieceCollisionVolumeData(unitID, piece("flare"), false,
    0,0,0, 0,0,0, 4, 0)
```

Reading a volume back (10 return values, type is numeric):

```lua
local sx,sy,sz, ox,oy,oz, vType, testType, pAxis, ignoreHits =
    Spring.GetUnitPieceCollisionVolumeData(unitID, pieceIndex)
```

`/debugcolvol` draws polygon volumes as a wireframe of the same triangles
the collision test traces, through the same matrix — so if the outline
does not sit on the mesh, neither do the hits.

### Design notes

- **Scales and offsets are ignored** for this type. Offsets are forced to
  zero in `InitShape()` so a stray value cannot shift the volume away
  from the mesh it is supposed to follow.
- **No cached geometry pointer.** The piece is passed to the intersection
  routine at test time rather than stored on the volume, so there is
  nothing to dangle across savegame loads, model swaps or volume copies.
- **The object-level volume cannot be polygon** — it has no piece behind
  it. It stays a sphere and defers to the piece tree, the same way
  `usePieceCollisionVolumes` already works.
- **Broad-phase.** The axis-scale box reject is meaningless for this type,
  so a slab test against the piece's own `mins`/`maxs` is used instead;
  rays nowhere near a piece never reach the per-triangle loop. A bounding
  sphere would be cheaper per test but fits elongated pieces poorly — a
  gun barrel's sphere has a radius of half its length and is mostly empty,
  so shots passing alongside it would still pay for a full triangle scan.
- **Discrete (point-in-volume) testing is not implemented.** It is also
  unreachable: `DetectHit()` branches to `IntersectPieceTree()` when
  `DefaultToPieceTree()` is set, before the continuous/discrete choice is
  made, and that branch overrides `forceTrace`, which itself overrides
  `testType`.

### Cost

Cost scales with triangle count per piece: a box is closed-form maths, a
polygon is a loop. Low-poly collision shells are recommended. The
broad-phase reject keeps rays that miss entirely out of the loop, but a
detailed mesh will still cost more than a primitive.

Tracing reads from a positions-only array built once per piece at model
load, rather than from the interleaved vertex buffer. A full `SVertexData`
is 80 bytes — position, normal, two tangents, bone weights, UVs — of which
the ray test uses the 12 bytes of position; reading whole vertices would
waste most of every cache line it touches.

### Changed files

```
rts/Sim/Misc/CollisionVolume.{h,cpp}
rts/Sim/Misc/CollisionHandler.{h,cpp}
rts/Sim/Objects/SolidObject.{h,cpp}
rts/Sim/Objects/SolidObjectDef.{h,cpp}
rts/Sim/Units/Unit.cpp
rts/Sim/Features/Feature.cpp
rts/Rendering/DebugColVolDrawer.cpp
rts/Rendering/Models/3DModelPiece.{hpp,cpp}
rts/Rendering/Models/IModelParser.cpp
rts/Lua/LuaSyncedCtrl.cpp
rts/Lua/LuaSyncedRead.cpp
rts/Lua/LuaUtils.cpp
```

### Building

Same as upstream:

```bash
bash docker-build-v2/build.sh windows
```

Output lands in `build-amd64-windows/install/`.

### Status

Based on upstream commit `e9d199329538cd9078a621c5e0f9ada416e1229f` (2026-09-18).

Tested on Windows. Verified in game: units spawn, polygon outlines follow
the real geometry on angled pieces, hits register per piece, and mixed
volume types coexist on one unit. Not benchmarked under heavy unit counts;
no automated tests added.

**v2** adds two performance changes that leave behaviour untouched: the
positions-only tracing array described under *Cost*, and the slab-test
broad-phase described under *Design notes*. Both were verified to produce
identical outlines and identical hit results.

### AI disclosure

Parts of this work were written with AI assistance (Claude). Per the
upstream [AI policy](AI_POLICY.md), this is disclosed here and in any
discussion or pull request derived from it.

### License

GPL v2 or later, inherited from the upstream engine. See `LICENSE`.

---
---

## Русский

### Проблема

Recoil задаёт каждому piece'у объём по умолчанию как axis-aligned коробку
по габаритам его вершин — одинаково в `ModelUtils.cpp`, `S3OParser.cpp` и
`3DOParser.cpp`:

```cpp
piece->SetCollisionVolume(CollisionVolume('b', 'z',
    piece->maxs - piece->mins,
    (piece->maxs + piece->mins) * 0.5f));
```

Если деталь стоит под углом — скошенная броня, наклонная плита,
повёрнутый модуль — такая коробка заметно больше самой детали и торчит в
пустоту. Выстрелы засчитываются там, где ничего нет.

Параметра поворота у коллизионных объёмов нет, так что подогнать коробку
под наклон нельзя. Единственный доступный обходной путь сегодня —
поворачивать сам piece из скрипта, но это разворачивает и видимый меш и
плохо масштабируется, когда деталей с разными углами много.

### Решение

Вместо второго источника ориентации добавлен тип объёма, форма которого
**и есть** геометрия: `COLVOL_TYPE_POLYGON`.

Ориентация приходит от самого piece'а, через тот же
`LocalModelPiece::GetModelSpaceMatrix()`, которым пользуются все
остальные типы. Вершины piece'а уже лежат в его локальном пространстве
(`ModelUtils.cpp`: *«transform model space mesh vertices into bone/piece
space»*) — ровно в том, в котором работает
`CCollisionHandler::Intersect()`, поэтому никаких дополнительных
преобразований не вводится.

### Использование

В UnitDef:

```lua
collisionVolumeType = "polygon",
```

Этого достаточно: движок переводит все piece'ы юнита на полигональные
объёмы и сам включает проверку по дереву piece'ов —
`usePieceCollisionVolumes` подразумевается и дописывать его не нужно.

Для **смешанных** конфигураций `collisionVolumeType` не указывают вообще,
а пишут

```lua
usePieceCollisionVolumes = true,
```

и назначают объёмы поштучно из скрипта юнита или гейджа:

```lua
-- типы объёмов: 0 эллипсоид, 1 цилиндр, 2 коробка, 3 сфера, 4 полигон
Spring.SetUnitPieceCollisionVolumeData(unitID, piece("hull"), true,
    0,0,0, 0,0,0, 4, 0)       -- полигон: размеры и смещение не используются

Spring.SetUnitPieceCollisionVolumeData(unitID, piece("turret"), true,
    30,20,30, 0,10,0, 2, 0)   -- коробка: размеры и смещение работают
```

Исключение piece'а из хит-теста делается как и для любого другого типа —
третьим аргументом:

```lua
Spring.SetUnitPieceCollisionVolumeData(unitID, piece("flare"), false,
    0,0,0, 0,0,0, 4, 0)
```

Чтение объёма обратно (10 возвращаемых значений, тип — число):

```lua
local sx,sy,sz, ox,oy,oz, vType, testType, pAxis, ignoreHits =
    Spring.GetUnitPieceCollisionVolumeData(unitID, pieceIndex)
```

`/debugcolvol` рисует полигональный объём по тем же треугольникам и той же
матрице, которыми пользуется сам тест столкновений: если контур не лежит
на меше, то и попадания считаются не там.

### Замечания по устройству

- **Размеры и смещение игнорируются.** Смещение принудительно обнуляется
  в `InitShape()`, чтобы случайно переданное значение не увело объём в
  сторону от геометрии, которую он должен повторять.
- **Указатель на геометрию не кэшируется.** Piece передаётся в тест
  столкновений в момент вызова, а не хранится в объёме — поэтому нечему
  повиснуть после загрузки сейва, смены модели или копирования объёма.
- **Объём уровня юнита полигональным быть не может** — за ним нет
  piece'а. Он остаётся сферой и делегирует в дерево piece'ов, ровно как
  штатный `usePieceCollisionVolumes`.
- **Ранний отсев.** Проверка по коробке из осевых размеров для этого типа
  бессмысленна, поэтому используется слэб-тест по собственным
  `mins`/`maxs` piece'а: лучи, проходящие далеко, до перебора
  треугольников не доходят. Ограничивающая сфера обошлась бы дешевле, но
  плохо облегает вытянутые детали — у ствола пушки её радиус равен
  половине длины и внутри в основном пустота, так что выстрелы,
  пролетающие вдоль него, всё равно оплачивали бы полный перебор.
- **Дискретный тест «точка внутри» не реализован.** Он же и недостижим:
  `DetectHit()` уходит в `IntersectPieceTree()` при включённом
  `DefaultToPieceTree()` — раньше, чем выбирается непрерывный или
  дискретный режим, и эта ветка перекрывает `forceTrace`, который сам
  перекрывает `testType`.

### Цена

Стоимость растёт с числом треугольников на piece: коробка считается
формулой, полигон — циклом. Рекомендуются низкополигональные
коллизионные «скорлупы». Ранний отсев убирает из цикла лучи, проходящие
мимо, но детальный меш всё равно обойдётся дороже примитива.

Трассировка читает из массива, содержащего только позиции, который
строится один раз на piece при загрузке модели, а не из общего вершинного
буфера. Полный `SVertexData` весит 80 байт — позиция, нормаль, два
тангенса, веса костей, UV — из которых тесту луча нужны 12 байт позиции;
чтение вершин целиком тратило бы впустую большую часть каждой
прочитанной кэш-линии.

### Изменённые файлы

```
rts/Sim/Misc/CollisionVolume.{h,cpp}
rts/Sim/Misc/CollisionHandler.{h,cpp}
rts/Sim/Objects/SolidObject.{h,cpp}
rts/Sim/Objects/SolidObjectDef.{h,cpp}
rts/Sim/Units/Unit.cpp
rts/Sim/Features/Feature.cpp
rts/Rendering/DebugColVolDrawer.cpp
rts/Rendering/Models/3DModelPiece.{hpp,cpp}
rts/Rendering/Models/IModelParser.cpp
rts/Lua/LuaSyncedCtrl.cpp
rts/Lua/LuaSyncedRead.cpp
rts/Lua/LuaUtils.cpp
```

### Сборка

Как в апстриме:

```bash
bash docker-build-v2/build.sh windows
```

Результат — в `build-amd64-windows/install/`.

### Состояние

Основано на коммите апстрима `e9d199329538cd9078a621c5e0f9ada416e1229f` (2026-09-18).

Проверено на Windows. В игре подтверждено: юниты спавнятся, контуры
полигональных объёмов повторяют реальную геометрию на наклонных деталях,
попадания засчитываются по конкретному piece'у, разные типы объёмов
уживаются на одном юните. Под нагрузкой из десятков юнитов не измерялось,
автоматических тестов не добавлено.

**v2** добавляет две правки производительности, не меняющие поведение:
массив только с позициями для трассировки (см. «Цена») и слэб-тест в
раннем отсеве (см. «Замечания по устройству»). Обе проверены: контуры и
результаты попаданий остались идентичными.

### Раскрытие использования ИИ

Часть работы выполнена с помощью ИИ (Claude). Согласно
[политике апстрима](AI_POLICY.md) это раскрывается здесь и в любом
обсуждении или pull request, производном от этой работы.

### Лицензия

GPL v2 или более поздняя, унаследована от апстрима. См. `LICENSE`.
