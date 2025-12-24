#version 130
//#extension GL_ARB_explicit_attrib_location : require

in vec3 pos;
in vec3 uvw;
in vec4 uvInfo;
in vec3 aparams;
in vec4 color;
in uvec2 dparams;

out vec4 vCol;
centroid out vec4 vUV;
out float vLayer;
out float vBF;
out float fragDist;
out float fogFactor;
out vec3 wsPos;
out vec4 vsPos;
out vec4 vsDistParam2;
out vec2 vsDistParam1;
noperspective out vec2 screenUV;

out float gl_ClipDistance[1];

uniform vec2 fogParams;
uniform vec3 camPos;
uniform vec4 clipPlane = vec4(0.0, 0.0, 0.0, 1.0);

#define NORM2SNORM(value) (value * 2.0 - 1.0)
#define SNORM2NORM(value) (value * 0.5 + 0.5)

uint GetUnpackedValue(uint packedValue, uint byteNum) {
	return (packedValue >> (8u * byteNum)) & 0xFFu;
}

void main() {
	// relative magnitude along X
	vsDistParam1.x = float(GetUnpackedValue(dparams.x, 0u)) / 255.0;
	// relative magnitude along Y
	vsDistParam1.y = float(GetUnpackedValue(dparams.x, 1u)) / 255.0;
	// relative frequency along X
	vsDistParam2.x = float(GetUnpackedValue(dparams.y, 0u)) / 255.0; vsDistParam2.x = NORM2SNORM(vsDistParam2.x);
	// relative frequency along Y
	vsDistParam2.y = float(GetUnpackedValue(dparams.y, 1u)) / 255.0; vsDistParam2.y = NORM2SNORM(vsDistParam2.y);
	// relative time mult along X
	vsDistParam2.z = float(GetUnpackedValue(dparams.y, 2u)) / 255.0; vsDistParam2.z = NORM2SNORM(vsDistParam2.z);
	// relative time mult along Y
	vsDistParam2.w = float(GetUnpackedValue(dparams.y, 3u)) / 255.0; vsDistParam2.w = NORM2SNORM(vsDistParam2.w);

	float ap = fract(aparams.z);

	float maxImgIdx = aparams.x * aparams.y - 1.0;
	ap *= maxImgIdx;

	float i0 = floor(ap);
	float i1 = i0 + 1.0;

	vBF = fract(ap); //blending factor

	if (maxImgIdx > 1.0) {
		vec2 uvDiff = (uvw.xy - uvInfo.xy);
		vUV = uvDiff.xyxy + vec4(
			floor(mod(i0 , aparams.x)),
			floor(   (i0 / aparams.x)),
			floor(mod(i1 , aparams.x)),
			floor(   (i1 / aparams.x))
		) * uvInfo.zwzw;
		vUV /= aparams.xyxy; //scale
		vUV += uvInfo.xyxy;
	} else {
		vUV = uvw.xyxy;
	}

	vLayer = uvw.z;
	vCol = color;
	wsPos = pos;

	fragDist = length(wsPos - camPos);
	fogFactor = (fogParams.y - fragDist) / (fogParams.y - fogParams.x);
	fogFactor = clamp(fogFactor, 0.0, 1.0);
	fogFactor = 1.0;

	gl_ClipDistance[0] = dot(vec4(wsPos, 1.0), clipPlane); //water clip plane

	// viewport relative UV [0.0, 1.0]
	vsPos = gl_ModelViewMatrix * vec4(wsPos, 1.0);
	gl_Position = gl_ProjectionMatrix * vsPos;
	screenUV = SNORM2NORM(gl_Position.xy / gl_Position.w);
}