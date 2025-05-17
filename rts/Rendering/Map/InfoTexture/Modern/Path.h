/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _PATH_TEXTURE_H
#define _PATH_TEXTURE_H

#include "ModernInfoTexture.h"
#include "Rendering/GL/FBO.h"
#include "System/Misc/SpringTime.h"


struct MoveDef;
struct UnitDef;


class CPathTexture : public CModernInfoTexture
{
public:
	CPathTexture();

public:
	void Update() override;
	bool IsUpdateNeeded() override;

	GLuint GetTexture() override;

	bool ShowMoveDef(const int pathType);
	bool ShowUnitDef(const int udefid);

private:
	const MoveDef* GetSelectedMoveDef();
	const UnitDef* GetCurrentBuildCmdUnitDef();

private:
	bool isCleared;
//	int updateFrame;
	int updateProcess;
	unsigned int lastSelectedPathType;
	int forcedPathType;
	int forcedUnitDef;
	spring_time lastUsage;
};

#endif // _PATH_TEXTURE_H
