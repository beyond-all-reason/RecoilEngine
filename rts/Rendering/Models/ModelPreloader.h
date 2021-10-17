#pragma once

#include "Rendering/Models/3DModel.h"
#include "Rendering/Models/3DModelVAO.h"

class ModelPreloader {
public:
	static void Load() {
		if (!enabled)
			return;

		// map features are loaded earlier in featureHandler.LoadFeaturesFromMap(); - not a big deal
		// Functions below cannot be multithreaded because modelLoader.LoadModel() deal with OpenGL functions
		LoadUnitDefs();
		LoadFeatureDefs();
		LoadWeaponDefs();

		// after that point we should've loaded all models, it's time to dispatch VBO/EBO/VAO creation
		S3DModelVAO::Init(); //TODO figure out where to put S3DModelVAO::Kill();
	}
	static void Clean() {
		S3DModelVAO::Kill();
	}
private:
	static constexpr bool enabled = true;
private:
	static void LoadUnitDefs();
	static void LoadFeatureDefs();
	static void LoadWeaponDefs();
};