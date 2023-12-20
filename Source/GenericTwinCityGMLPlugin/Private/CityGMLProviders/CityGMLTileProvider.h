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

#include "ICityGMLProvider.h"

class AGeoReferencingSystem;

namespace GenericTwin {
class ICityGMLImporter;
}

class CityGMLTileProvider	:	public ICityGMLProvider
{

public:

	CityGMLTileProvider(const FString &BasePath, AGeoReferencingSystem &GeoRefSystem);

	virtual ~CityGMLTileProvider();

	virtual bool Initialize() override;
	
	virtual FVector GetReferenceLocation() const override;

	virtual FVector2D GetTileSize() const override;

	virtual bool HasTile(const GenericTwin::TTileId &TileId) const override;

	virtual bool BeginLoading(const GenericTwin::TTileId &TileId) override;

	virtual FVector GetLoadedTileLocation() const override;

	virtual void ExtractBuildings(GenericTwin::SMeshSection &Walls, GenericTwin::SMeshSection &Roofs) override;

	virtual void EndLoading() override;

private:

	void BuildIndex();

	AGeoReferencingSystem							&m_GeoReferenceSystem;
	FString											m_BasePath;

	FIntVector2										m_ReferenceTileIndex;
	FVector											m_ReferenceLocation;
	FVector2D										m_TileSize;

	TMap<GenericTwin::TTileId, FString>				m_AvailableTiles;

	TSharedPtr<GenericTwin::ICityGMLImporter>		m_Importer;
	FIntVector2										m_CurrentTileIndex;


};
