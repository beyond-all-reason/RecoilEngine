#pragma once

#include <limits>
#include "System/float3.h"

static constexpr int MAX_MODEL_OBJECTS  = 3840;
static constexpr int AVG_MODEL_PIECES   = 16; // as it used to be
static constexpr int MAX_PIECES_PER_MODEL = std::numeric_limits<uint16_t>::max() - 1;
static constexpr int INV_PIECE_NUM = MAX_PIECES_PER_MODEL + 1; // invalid piece

static constexpr float3 DEF_MIN_SIZE( 10000.0f,  10000.0f,  10000.0f);
static constexpr float3 DEF_MAX_SIZE(-10000.0f, -10000.0f, -10000.0f);

enum ModelType {
	MODELTYPE_3DO    = 0,
	MODELTYPE_S3O    = 1,
	MODELTYPE_ASS    = 2, // Assimp
	MODELTYPE_CNT    = 3  // count
};