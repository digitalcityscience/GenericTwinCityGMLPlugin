/**

The MIT License (MIT)

Copyright (c) 2023 - present, Hamburg Port Authority


Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the “Software”), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

**/

#pragma once

#include "CoreMinimal.h"

namespace GenericTwin {

struct GENERICTWINCITYGMLIMPORT_API SCityGMLVertex
{
	SCityGMLVertex(const FVector &pos)
		:	position(pos)
		,	normal(0.0f, 0.0f, 0.0f)
		,	texture_coordinate(0.0f, 0.0f)
	{
	}

	SCityGMLVertex(const FVector &pos, const FVector2D &texCoord)
		:	position(pos)
		,	normal(0.0f, 0.0f, 0.0f)
		,	texture_coordinate(texCoord)
	{
	}

	FVector			position;
	FVector			normal;
	FVector2D		texture_coordinate;
};

struct GENERICTWINCITYGMLIMPORT_API SCityGMLSurface
{
	enum class Type	: uint32
	{
		Unknown,
		Wall,
		Roof,
		Ground,
		Closure,
		Floor,
		InteriorWall,
		Ceiling,
		OuterCeiling,
		OuterFloor
	};

	FString						surface_id;
	Type						surface_type;

	FString						texture_url;
	FLinearColor				base_color;

	TArray<SCityGMLVertex>		vertices;
	TArray<int32>				indices;

	void CalculateNormals();
};

struct GENERICTWINCITYGMLIMPORT_API SCityGMLBuilding
{
	FString						building_id;
	FVector						bounding_min;
	FVector						bounding_max;

	TArray<SCityGMLSurface>		surfaces;

};

}	//	GenericTwin
