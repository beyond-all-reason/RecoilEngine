/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef S3O_PARSER_H
#define S3O_PARSER_H

#include "3DModelPiece.hpp"
#include "IModelParser.h"

#include "System/type2.h"
#include "System/Threading/SpringThreading.h"

enum {
	S3O_PRIMTYPE_TRIANGLES      = 0,
	S3O_PRIMTYPE_TRIANGLE_STRIP = 1,
	S3O_PRIMTYPE_QUADS          = 2,
};


struct SS3OPiece: public S3DModelPiece {
public:
	SS3OPiece() = default;
	SS3OPiece(const SS3OPiece&) = delete;
	SS3OPiece(SS3OPiece&& p) noexcept = delete;

	SS3OPiece& operator = (const SS3OPiece& p) = delete;
	SS3OPiece& operator = (SS3OPiece&& p) noexcept = delete;
public:
	void Trianglize();
	void SetVertexTangents();

public:
	int primType = S3O_PRIMTYPE_TRIANGLES;
};



class CS3OParser: public TypedModelParser<SS3OPiece>
{
public:
	void Init() override {};
	void Kill() override;

	void Load(S3DModel& model, const std::string& name) override;

private:
	SS3OPiece* AllocPiece();
	SS3OPiece* LoadPiece(S3DModel*, SS3OPiece*, std::vector<uint8_t>& buf, int offset);
};

#endif /* S3O_PARSER_H */
