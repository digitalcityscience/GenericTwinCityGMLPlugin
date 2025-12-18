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

#include "Common/BaseTypes.h"

#include "CityGMLImporterTypes.h"

#include "Common/Caching/CachedFile.h"

DECLARE_LOG_CATEGORY_EXTERN(LogCityGMLBuildingMeshGenerator, Log, All);

namespace GenericTwin {
class FTextureAtlas;
class FCacheKey;
}

class UTexture2DArray;

class GENERICTWINCITYGMLPLUGIN_API FCityGMLBuildingMeshGenerator
{
private:

	struct STextureData
	{
		int32		Width;
		int32		Height;
		uint32		Area;

		FString		Url;
	};

	struct SCachedMeshSection
	{
		int32		NumVertices = 0;
		int32		NumNormals = 0;
		int32		NumUV0 = 0;
		int32		NumUV1 = 0;
		int32		NumColors = 0;
		int32		NumIndices = 0;

		void Fill(const GenericTwin::SMeshSection& MeshSection);

	};

	struct SCacheHeader
	{
		enum
		{
			CurrentVersion = 1
		};

		uint32					Checksum;

		int32					Version = CurrentVersion;

		FVector					LowerLeft;

		int32					NumAtlases = 0;
		int32					AtlasFormat = 0;
		int32					AtlasSize = 0;
		int32					AtlasDataSize = 0;

		SCachedMeshSection		TexturedSection;
		SCachedMeshSection		RoofSection;
		SCachedMeshSection		WallSection;

	};


	typedef TSharedPtr<GenericTwin::FTextureAtlas> TextureAtlasPtr;

public:

	FCityGMLBuildingMeshGenerator(float WallThreshold, const FString &BasePath, int32 AtlasSize);

	~FCityGMLBuildingMeshGenerator();

	void Build(const TArray<GenericTwin::SCityGMLBuilding> &Buildings, bool KeepTextured);

	void CompressTextures(bool WithAlphaChannel);

	const GenericTwin::SMeshSection& GetTexturedMeshSection() const;
	const GenericTwin::SMeshSection& GetRoofMeshSection() const;
	const GenericTwin::SMeshSection& GetWallMeshSection() const;

	GenericTwin::TextureProxyPtr GetAtlasTextureArray() const;

	void SaveToCache(const FString &Url, const FVector &LowerLeft);
	bool LoadFromCache(const FString &Url, FVector &LowerLeft);

#if WITH_EDITOR

	TArray<GenericTwin::SImagePtr> GetAtlasImages() const;

#endif

private:

	void BuildTextureAtlases(const TArray<GenericTwin::SCityGMLBuilding> &Buildings);

	void AddTexturedSurface(const GenericTwin::SCityGMLSurface &Surface);
	void AddUntexturedSurface(const GenericTwin::SCityGMLSurface &Surface);

	FString								m_BasePath;

	int32								m_AtlasSize = 0;

	TArray<TextureAtlasPtr>				m_Atlases;

	TMap<FString, int32>				m_AtlasMap;

	const float							m_WallThreshold;

	GenericTwin::SMeshSection			m_TexturedSection;
	GenericTwin::SMeshSection			m_RoofSection;
	GenericTwin::SMeshSection			m_WallSection;

	GenericTwin::GenericAssetFilePtr	m_CachedAssetFile;
	uint32								m_NumCachedAtlases = 0;
	EPixelFormat						m_CachedPixelFormat;
	uint32								m_CachedAtlasSize = 0;
	uint32								m_CachedAtlasDataSize = 0;
	const void							*m_CachedTextureData = 0;

};


inline const GenericTwin::SMeshSection& FCityGMLBuildingMeshGenerator::GetTexturedMeshSection() const
{
	return m_TexturedSection;
}

inline const GenericTwin::SMeshSection& FCityGMLBuildingMeshGenerator::GetRoofMeshSection() const
{
	return m_RoofSection;
}

inline const GenericTwin::SMeshSection& FCityGMLBuildingMeshGenerator::GetWallMeshSection() const
{
	return m_WallSection;
}

