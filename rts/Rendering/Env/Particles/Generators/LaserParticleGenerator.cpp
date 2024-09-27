#include "LaserParticleGenerator.h"

CR_BIND(LaserParticleData, )
CR_REG_METADATA(LaserParticleData,
(
	CR_MEMBER(pos),
	CR_MEMBER(curLength),
	CR_MEMBER(dir),
	CR_MEMBER(maxLength),
	CR_MEMBER(thickness),
	CR_MEMBER(coreThickness),
	CR_MEMBER(color1),
	CR_MEMBER(color2),
	CR_MEMBER(lodDistance),
	CR_MEMBER(drawOrder),
	CR_MEMBER(checkCol),
	CR_MEMBER(stayTime),
	CR_MEMBER(speed),
	CR_MEMBER(texCoord1),
	CR_MEMBER(texCoord2)
))