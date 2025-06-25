---@meta

--------------------------------------------------------------------------------
-- Vectors and Matrices
--------------------------------------------------------------------------------

---Cartesian double (XY)
---
---@class xy
---@field [1] number x
---@field [2] number y

---Cartesian triple (XYZ)
---
---@class xyz
---@field [1] number x
---@field [2] number y
---@field [3] number z

---Cartesian quadruple (XYZW)
---
---@class xyzw
---@field [1] number x
---@field [2] number y
---@field [3] number z
---@field [4] number w

---@alias float2 [number, number]
---@alias float3 [number, number, number]
---@alias float4 [number, number, number, number]

---@class Matrix2x2
---@field [1] number Element at [1,1]
---@field [2] number Element at [1,2]
---@field [3] number Element at [2,1]
---@field [4] number Element at [2,2]
---@class Matrix3x3
---@field [1] number Element at [1,1]
---@field [2] number Element at [1,2]
---@field [3] number Element at [1,3]
---@field [4] number Element at [2,1]
---@field [5] number Element at [2,2]
---@field [6] number Element at [2,3]
---@field [7] number Element at [3,1]
---@field [8] number Element at [3,2]
---@field [9] number Element at [3,3]
---@class Matrix4x4
---@field [1] number Element at [1,1]
---@field [2] number Element at [1,2]
---@field [3] number Element at [1,3]
---@field [4] number Element at [1,4]
---@field [5] number Element at [2,1]
---@field [6] number Element at [2,2]
---@field [7] number Element at [2,3]
---@field [8] number Element at [2,4]
---@field [9] number Element at [3,1]
---@field [10] number Element at [3,2]
---@field [11] number Element at [3,3]
---@field [12] number Element at [3,4]
---@field [13] number Element at [4,1]
---@field [14] number Element at [4,2]
---@field [15] number Element at [4,3]
---@field [16] number Element at [4,4]

--------------------------------------------------------------------------------
-- Color
--------------------------------------------------------------------------------

---Color triple (RGB)
---
---@class rgb
---@field [1] number Red value.
---@field [2] number Green value.
---@field [3] number Blue value.

---Color quadruple (RGBA)
---
---@class rgba
---@field [1] number Red value.
---@field [2] number Green value.
---@field [3] number Blue value.
---@field [4] number Alpha value.

---Indicator bytes representing color code operations during font rendering
---
---@class TextColorCode
---@field Color string Indicates that the following bytes contain color code information
---@field ColorAndOutline string Indicates that the following bytes contain color code and outline information
---@field Reset string Indicates reset of the current color

--------------------------------------------------------------------------------
-- Camera
--------------------------------------------------------------------------------

---@alias CameraMode
---| 0 # fps
---| 1 # ta
---| 2 # spring
---| 3 # rot
---| 4 # free
---| 5 # ov
---| 6 # dummy

---@alias CameraName
---| "ta"
---| "spring"
---| "rot"
---| "ov"
---| "free"
---| "fps"
---| "dummy"

---Parameters for camera state.
---
---@class CameraState
---
---Highly dependent on the type of the current camera controller.
---
---@field name CameraName The camera name.
---@field mode CameraMode The camera mode.
---@field fov number?
---@field px number? Position X of the ground point in screen center.
---@field py number? Position Y of the ground point in screen center.
---@field pz number? Position Z of the ground point in screen center.
---@field dx number? Camera direction vector X.
---@field dy number? Camera direction vector Y.
---@field dz number? Camera direction vector Z.
---@field rx number? Camera rotation angle on X axis. (spring)
---@field ry number? Camera rotation angle on Y axis. (spring)
---@field rz number? Camera rotation angle on Z axis. (spring)
---@field angle number? Camera rotation angle on X axis (aka tilt/pitch). (ta)
---@field flipped number? `-1` for when south is down, `1` for when north is down. (ta)
---@field dist number? Camera distance from the ground. (spring)
---@field height number? Camera distance from the ground. (ta)
---@field oldHeight number? Camera distance from the ground, cannot be changed. (rot)

--------------------------------------------------------------------------------
-- Resources
--------------------------------------------------------------------------------

---@alias ResourceName "metal"|"energy"|"m"|"e"

---@alias StorageName "metalStorage"|"energyStorage"|"ms"|"es"

---@alias ResourceUsage table<ResourceName, number>
