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

#include "SpatialPartitioning/TileManager/TileBase.h"

#include "StreamingCityGMLTile.generated.h" 


namespace GenericTwin {
struct SMeshSection;
}	//	namespace GenericTwin

class UStreamingCityGMLLayer;
class AGenericTwinStaticMeshActor;

UCLASS()
class UStreamingCityGMLTile	:	public UTileBase
{
	GENERATED_BODY()

public:

	UStreamingCityGMLTile();

	void SetLayer(UStreamingCityGMLLayer *Layer);

	virtual ~UStreamingCityGMLTile();

	virtual void Shutdown() override;

	virtual bool Load() override;
	virtual void Unload() override;

	virtual void SetIsHidden(bool IsHidden) override;

	virtual void OnTileVisibilityChanged() override;

	void AddToWorld(const TObjectPtr<AGenericTwinStaticMeshActor> &Actor);

	void UpdatedHeightOffset(float HeightOffset);

private:

	UStreamingCityGMLLayer			*m_Layer = 0;

	UPROPERTY()
	AGenericTwinStaticMeshActor		*m_Actor = 0;

};


inline void UStreamingCityGMLTile::SetLayer(UStreamingCityGMLLayer *Layer)
{
	m_Layer = Layer;
}
