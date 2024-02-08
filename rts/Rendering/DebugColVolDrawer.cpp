/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "DebugColVolDrawer.h"

#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Map/ReadMap.h"
#include "Rendering/Units/UnitDrawer.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/glExtra.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Models/3DModel.h"
#include "Sim/Features/Feature.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitTypes/Factory.h"
#include "Sim/Weapons/PlasmaRepulser.h"
#include "Sim/Weapons/Weapon.h"
#include "System/UnorderedSet.hpp"

static constexpr float4 DEFAULT_COLVOL_COLOR = float4(0.45f, 0.00f, 0.45f, 0.35f); // purple
static constexpr float4 DEFAULT_SELVOL_COLOR = float4(0.00f, 0.45f, 0.00f, 0.20f); // dark green
static unsigned int volumeDisplayListIDs[] = {0, 0, 0, 0, 0};

static inline void DrawCollisionVolume(const CollisionVolume* vol)
{
	glPushMatrix();

	switch (vol->GetVolumeType()) {
		case CollisionVolume::COLVOL_TYPE_ELLIPSOID:
		case CollisionVolume::COLVOL_TYPE_SPHERE: {
			// scaled sphere is special case of ellipsoid: radius, slices, stacks
			glTranslatef(vol->GetOffset(0), vol->GetOffset(1), vol->GetOffset(2));
			glScalef(vol->GetHScale(0), vol->GetHScale(1), vol->GetHScale(2));
			glWireSphere(&volumeDisplayListIDs[0], 20, 20);
		} break;
		case CollisionVolume::COLVOL_TYPE_CYLINDER: {
			// scaled cylinder: base-radius, top-radius, height, slices, stacks
			//
			// (cylinder base is drawn at unit center by default so add offset
			// by half major axis to visually match the mathematical situation,
			// height of the cylinder equals the unit's full major axis)
			switch (vol->GetPrimaryAxis()) {
				case CollisionVolume::COLVOL_AXIS_X: {
					glTranslatef(-(vol->GetHScale(0)), 0.0f, 0.0f);
					glTranslatef(vol->GetOffset(0), vol->GetOffset(1), vol->GetOffset(2));
					glScalef(vol->GetScale(0), vol->GetHScale(1), vol->GetHScale(2));
					glRotatef( 90.0f, 0.0f, 1.0f, 0.0f);
				} break;
				case CollisionVolume::COLVOL_AXIS_Y: {
					glTranslatef(0.0f, -(vol->GetHScale(1)), 0.0f);
					glTranslatef(vol->GetOffset(0), vol->GetOffset(1), vol->GetOffset(2));
					glScalef(vol->GetHScale(0), vol->GetScale(1), vol->GetHScale(2));
					glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
				} break;
				case CollisionVolume::COLVOL_AXIS_Z: {
					glTranslatef(0.0f, 0.0f, -(vol->GetHScale(2)));
					glTranslatef(vol->GetOffset(0), vol->GetOffset(1), vol->GetOffset(2));
					glScalef(vol->GetHScale(0), vol->GetHScale(1), vol->GetScale(2));
				} break;
			}

			glWireCylinder(&volumeDisplayListIDs[1], 20, 1.0f);
		} break;
		case CollisionVolume::COLVOL_TYPE_BOX: {
			// scaled cube: length, width, height
			glTranslatef(vol->GetOffset(0), vol->GetOffset(1), vol->GetOffset(2));
			glScalef(vol->GetScale(0), vol->GetScale(1), vol->GetScale(2));
			glWireCube(&volumeDisplayListIDs[2]);
		} break;
	}

	glPopMatrix();
}



/*
static void DrawUnitDebugPieceTree(const LocalModelPiece* lmp, const LocalModelPiece* lap, int lapf)
{
	glPushMatrix();
		glMultMatrixf(lmp->GetModelSpaceMatrix());

		if (lmp->GetScriptVisible() && !lmp->GetCollisionVolume()->IgnoreHits()) {
			if ((lmp == lap) && (lapf > 0 && ((gs->frameNum - lapf) < 150))) {
				glColor3f((1.0f - ((gs->frameNum - lapf) / 150.0f)), 0.0f, 0.0f);
			}

			// factors in the volume offsets
			DrawCollisionVolume(lmp->GetCollisionVolume());

			if ((lmp == lap) && (lapf > 0 && ((gs->frameNum - lapf) < 150))) {
				glColorf3(DEFAULT_COLVOL_COLOR);
			}
		}
	glPopMatrix();

	for (unsigned int i = 0; i < lmp->children.size(); i++) {
		DrawUnitDebugPieceTree(lmp->children[i], lap, lapf);
	}
}
*/

static void DrawObjectDebugPieces(const CSolidObject* o)
{
	const int hitDeltaTime = gs->frameNum - o->pieceHitFrames[true];
	const int setFadeColor = (o->pieceHitFrames[true] > 0 && hitDeltaTime < 150);

	for (unsigned int n = 0; n < o->localModel.pieces.size(); n++) {
		const LocalModelPiece* lmp = o->localModel.GetPiece(n);
		const CollisionVolume* lmpVol = lmp->GetCollisionVolume();

		if (!lmp->GetScriptVisible() || lmpVol->IgnoreHits())
			continue;

		if (setFadeColor && lmp == o->hitModelPieces[true])
			glColor3f((1.0f - (hitDeltaTime / 150.0f)), 0.0f, 0.0f);

		glPushMatrix();
		glMultMatrixf(lmp->GetModelSpaceMatrix());
		// factors in the volume offsets
		DrawCollisionVolume(lmpVol);
		glPopMatrix();

		if (setFadeColor && lmp == o->hitModelPieces[true])
			glColorf4(DEFAULT_COLVOL_COLOR);
	}
}



static inline void DrawObjectMidAndAimPos(const CSolidObject* o)
{
	GLUquadricObj* q = gluNewQuadric();
	glDisable(GL_DEPTH_TEST);

	if (o->aimPos != o->midPos) {
		// draw the aim-point
		glPushMatrix();
		glTranslatef3(o->relAimPos);
		glColor4f(1.0f, 0.0f, 0.0f, 0.35f);
		gluQuadricDrawStyle(q, GLU_FILL);
		gluSphere(q, 2.0f, 5, 5);
		glPopMatrix();
	}

	{
		// draw the mid-point, keep this transform on the stack
		glTranslatef3(o->relMidPos);
		glColor4f(1.0f, 0.0f, 1.0f, 0.35f);
		gluQuadricDrawStyle(q, GLU_FILL);
		gluSphere(q, 2.0f, 5, 5);
	}

	glEnable(GL_DEPTH_TEST);
	gluDeleteQuadric(q);
}



static inline void DrawFeatureColVol(const CFeature* f)
{
	const CollisionVolume* v = &f->collisionVolume;

	if (f->IsInVoid())
		return;
	if (!f->IsInLosForAllyTeam(gu->myAllyTeam) && !gu->spectatingFullView)
		return;
	if (!camera->InView(f->pos, f->GetDrawRadius()))
		return;

	glPushMatrix();
		glMultMatrixf(f->GetTransformMatrixRef(false));
		DrawObjectMidAndAimPos(f);

		glColorf4(DEFAULT_SELVOL_COLOR);
		DrawCollisionVolume(&f->selectionVolume);

		glColorf4(DEFAULT_COLVOL_COLOR);
		if (v->DefaultToPieceTree()) {
			// draw only the piece volumes for less clutter
			// note: relMidPos transform is on the stack at this
			// point but all piece-positions are relative to pos
			// --> undo it
			glTranslatef3(-f->relMidPos);
			DrawObjectDebugPieces(f);
			glTranslatef3(f->relMidPos);
		} else {
			if (!v->IgnoreHits()) {
				DrawCollisionVolume(v);
			}
		}

		if (v->HasCustomType() || v->HasCustomProp(f->radius)) {
			// assume this is a custom volume; draw radius-sphere next to it
			glColor4f(0.5f, 0.5f, 0.5f, 0.35f);
			glScalef(f->radius, f->radius, f->radius);
			glWireSphere(&volumeDisplayListIDs[0], 20, 20);
		}
	glPopMatrix();
}

static inline void DrawUnitColVol(const CUnit* u)
{
	if (u->IsInVoid())
		return;
	if (!(u->losStatus[gu->myAllyTeam] & LOS_INLOS) && !gu->spectatingFullView)
		return;
	if (!camera->InView(u->drawMidPos, u->GetDrawRadius()))
		return;

	const CollisionVolume* v = &u->collisionVolume;

	GLUquadricObj* q = gluNewQuadric();
	gluQuadricDrawStyle(q, GLU_FILL);
	glDisable(GL_DEPTH_TEST);

	for (const CWeapon* w: u->weapons) {
		glPushMatrix();
		glTranslatef3(w->aimFromPos);
		glColor4f(1.0f, 1.0f, 0.0f, 0.4f);
		gluSphere(q, 1.0f, 5, 5);
		glPopMatrix();

		glPushMatrix();
		glTranslatef3(w->weaponMuzzlePos);
		if (w->HaveTarget()) {
			glColor4f(1.0f, 0.8f, 0.0f, 0.4f);
		} else {
			glColor4f(1.0f, 0.0f, 0.0f, 0.4f);
		}
		gluSphere(q, 1.0f, 5, 5);
		glPopMatrix();

		if (w->HaveTarget()) {
			glPushMatrix();
			glTranslatef3(w->GetCurrentTargetPos());
			glColor4f(1.0f, 0.8f, 0.0f, 0.4f);
			gluSphere(q, 1.0f, 5, 5);
			glPopMatrix();
		}
	}

	glEnable(GL_DEPTH_TEST);
	gluDeleteQuadric(q);


	glPushMatrix();
		glMultMatrixf(u->GetTransformMatrix(false));
		DrawObjectMidAndAimPos(u);

		glColorf4(DEFAULT_SELVOL_COLOR);
		DrawCollisionVolume(&u->selectionVolume);

		glColorf4(DEFAULT_COLVOL_COLOR);
		if (v->DefaultToPieceTree()) {
			// draw only the piece volumes for less clutter
			// note: relMidPos transform is on the stack at this
			// point but all piece-positions are relative to pos
			// --> undo it
			glTranslatef3(-u->relMidPos);
			DrawObjectDebugPieces(u);
			glTranslatef3(u->relMidPos);
		} else {
			if (!v->IgnoreHits()) {
				const int hitDeltaTime = gs->frameNum - u->lastAttackFrame;
				const int setFadeColor = (u->lastAttackFrame > 0 && hitDeltaTime < 150);

				// make it fade red under attack
				if (setFadeColor)
					glColor3f(1.0f - (hitDeltaTime / 150.0f), 0.0f, 0.0f);

				// if drawing this, disable the DrawObjectMidAndAimPos call
				// DrawCollisionVolume((u->localModel).GetBoundingVolume());
				DrawCollisionVolume(v);

				if (setFadeColor)
					glColorf4(DEFAULT_COLVOL_COLOR);
			}
		}
		if (u->shieldWeapon != nullptr) {
			const CPlasmaRepulser* shieldWeapon = static_cast<const CPlasmaRepulser*>(u->shieldWeapon);
			const CollisionVolume* shieldColVol = &shieldWeapon->collisionVolume;

			glColor4f(0.0f, 0.0f, 0.6f, 0.35f);

			glPopMatrix();
			glPushMatrix();
			glMultMatrixf(CMatrix44f{shieldWeapon->weaponMuzzlePos});
			DrawCollisionVolume(shieldColVol);
			glPopMatrix();
			glPushMatrix();
			glMultMatrixf(u->GetTransformMatrix(false));
		}

		if (v->HasCustomType() || v->HasCustomProp(u->radius)) {
			// assume this is a custom volume; draw radius-sphere next to it
			glColor4f(0.5f, 0.5f, 0.5f, 0.35f);
			glPushMatrix();
			glScalef(u->radius, u->radius, u->radius);
			glWireSphere(&volumeDisplayListIDs[0], 20, 20);
			glPopMatrix();
		}

		if (const CFactory* f = dynamic_cast<const CFactory*>(u)) {
			if (f->boPerform) {
				glColor4f(0.0f, 1.0f, 1.0f, 0.35f);

				glPushMatrix();

				float3 boRelDir = (f->boRelHeading == 0) ? FwdVector : GetVectorFromHeading(f->boRelHeading % SPRING_MAX_HEADING);
				float3 boPos = f->pos + boRelDir * f->boOffset;

				glTranslatef3(boPos - f->pos); //because of glMultMatrixf(u->GetTransformMatrix(false));

				if (f->boSherical) {
					glScalef(f->boRadius, f->boRadius, f->boRadius);
					glWireSphere(&volumeDisplayListIDs[3], 20, 20);
				}
				else {
					glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
					glScalef(f->boRadius, f->boRadius, 1.0f);
					glWireCylinder(&volumeDisplayListIDs[4], 20, 20.0f);
				}

				glPopMatrix();
			}
		}

	glPopMatrix();
}


class CDebugColVolQuadDrawer : public CReadMap::IQuadDrawer {
public:
	void ResetState() { alreadyDrawnIds.clear(); }
	void DrawQuad(int x, int y)
	{
		const CQuadField::Quad& q = quadField.GetQuadAt(x, y);

		for (const CFeature* f: q.features) {
			if (alreadyDrawnIds.find(MAX_UNITS + f->id) == alreadyDrawnIds.end()) {
				alreadyDrawnIds.insert(MAX_UNITS + f->id);
				DrawFeatureColVol(f);
			}
		}

		for (const CUnit* u: q.units) {
			if (alreadyDrawnIds.find(u->id) == alreadyDrawnIds.end()) {
				alreadyDrawnIds.insert(u->id);
				DrawUnitColVol(u);
			}
		}
	}

	spring::unordered_set<int> alreadyDrawnIds;
};



namespace DebugColVolDrawer
{
	bool enable = false;

	void Draw()
	{
		if (!enable)
			return;

		glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
			glDisable(GL_LIGHTING);
			glDisable(GL_LIGHT0);
			glDisable(GL_LIGHT1);
			glDisable(GL_CULL_FACE);
			glDisable(GL_TEXTURE_2D);
			// glDisable(GL_BLEND);
			glDisable(GL_ALPHA_TEST);
			glDisable(GL_FOG);
			glDisable(GL_CLIP_PLANE0);
			glDisable(GL_CLIP_PLANE1);

			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			glLineWidth(2.0f);
			glDepthMask(GL_TRUE);

			static CDebugColVolQuadDrawer drawer;

			drawer.ResetState();
			readMap->GridVisibility(nullptr, &drawer, 1e9, CQuadField::BASE_QUAD_SIZE / SQUARE_SIZE);

			glLineWidth(1.0f);
		glPopAttrib();
	}
}
