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

#include "CityGMLProviders/CityGMLTileProvider.h"

#include "GenericTwinCityGMLImport.h"
#include "ICityGMLCoordinateTransform.h"

#include "CityGMLGeometryExtractor.h"

#include "Math/NumericLimits.h"

#include "GeoReferencingSystem.h"

CityGMLTileProvider::CityGMLTileProvider(const FString &BasePath, AGeoReferencingSystem &GeoRefSystem)
	:	m_GeoReferenceSystem(GeoRefSystem)
	,	m_BasePath(BasePath)
{

}

CityGMLTileProvider::~CityGMLTileProvider()
{
}

bool CityGMLTileProvider::Initialize()
{
	BuildIndex();
	return m_AvailableTiles.Num() > 0;
}

FVector CityGMLTileProvider::GetReferenceLocation() const
{
	return m_ReferenceLocation;
}

FVector2D CityGMLTileProvider::GetTileSize() const
{
	return m_TileSize;
}

bool CityGMLTileProvider::HasTile(const GenericTwin::TTileId &TileId) const
{
	return m_AvailableTiles.Contains(TileId);
}

bool CityGMLTileProvider::BeginLoading(const GenericTwin::TTileId &TileId)
{
	bool res = false;
	if(m_AvailableTiles.Contains(TileId))
	{
		m_Importer = FGenericTwinCityGMLImportModule::GetImporter();
		if	(	m_Importer
			&&	m_Importer->Load(m_AvailableTiles[TileId])
			)
		{
			m_CurrentTileIndex = FIntVector2(m_ReferenceTileIndex.X + TileId.GetTileX(), m_ReferenceTileIndex.Y + TileId.GetTileY());
			res = true;
		}
	}
	return res;
}

FVector CityGMLTileProvider::GetLoadedTileLocation() const
{
	FVector tileLocation;
	m_GeoReferenceSystem.ProjectedToEngine(FVector(m_CurrentTileIndex.X * 1000, m_CurrentTileIndex.Y * 1000, 0.0), tileLocation);

	return tileLocation;
}

void CityGMLTileProvider::ExtractBuildings(GenericTwin::SMeshSection &Walls, GenericTwin::SMeshSection &Roofs)
{
	if(m_Importer)
	{
		class Transform	: public GenericTwin::ICityGMLCoordinateTransform
		{
		public:
			Transform(const FVector &Offset)
				:	m_Offset(Offset)
			{	}

			virtual FVector operator() (const FVector &pos)
			{
				FVector transformed = 100.0 * (pos - m_Offset);
				return FVector(transformed.X, -transformed.Y, transformed.Z);
			}

		private:
			FVector					m_Offset;
		};

		CityGMLGeometryExtractor extractor(m_Importer);
		Transform transform(FVector(m_CurrentTileIndex.X * 1000.0, m_CurrentTileIndex.Y * 1000.0, 0.0));
		extractor.ExtractBuildings(transform, Walls, Roofs);

	}
}

void CityGMLTileProvider::EndLoading()
{
	m_Importer.Reset();
}

void CityGMLTileProvider::BuildIndex()
{
	struct STileIndexData
	{
		int32			TileX;
		int32			TileY;
		FString			FullPath;
	};

	FIntVector2 minTileIndex(TNumericLimits< int32 >::Max());
	FIntVector2 maxTileIndex(TNumericLimits< int32 >::Min());
	TArray<STileIndexData> tileIndexData;

	IFileManager& fileMgr = IFileManager::Get();
	TArray<FString> files;
	fileMgr.FindFiles(files, *m_BasePath, TEXT("xml"));
	for(const auto &file : files)
	{
		// LoD2_554_5927_1_HH
		TArray <FString> parts;
		if	(	file.ParseIntoArray(parts, TEXT("_"), true) == 5
			&&	parts[1].IsNumeric()
			&&	parts[2].IsNumeric()
			)
		{
			const int32 x = FCString::Atoi(*parts[1]);
			const int32 y = FCString::Atoi(*parts[2]);

			tileIndexData.Add( {x, y, FPaths::Combine(m_BasePath, TEXT("/"), file)} );

			if(x < minTileIndex.X) minTileIndex.X = x;
			if(y < minTileIndex.Y) minTileIndex.Y = y;
			if(x > maxTileIndex.X) maxTileIndex.X = x;
			if(y > maxTileIndex.Y) maxTileIndex.Y = y;
		}
	}

	m_ReferenceTileIndex.X = minTileIndex.X + (maxTileIndex.X - minTileIndex.X) / 2;
	m_ReferenceTileIndex.Y = minTileIndex.Y + (maxTileIndex.Y - minTileIndex.Y) / 2;
	for(const auto &idxData : tileIndexData)
	{
		GenericTwin::TTileId tileId(idxData.TileX - m_ReferenceTileIndex.X, idxData.TileY - m_ReferenceTileIndex.Y);
		m_AvailableTiles.Add(tileId, idxData.FullPath);
	}

	//	Todo: take this value from config and/or determine from file(s)
	m_GeoReferenceSystem.ProjectedToEngine(FVector(m_ReferenceTileIndex.X * 1000, m_ReferenceTileIndex.Y * 1000, 0.0), m_ReferenceLocation);
	m_TileSize = 100.0 * FVector2D(1000.0, 1000.0);
}
