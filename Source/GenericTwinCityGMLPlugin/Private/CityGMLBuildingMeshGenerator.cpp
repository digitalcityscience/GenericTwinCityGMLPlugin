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

#include "CityGMLBuildingMeshGenerator.h"
#include "Import/ImageImporter.h"
#include "Import/ImageImportConfiguration.h"
#include "Common/Image/TextureAtlas.h"

#include "Common/Material/GenericTwinTextureCache.h"
#include "Common/Material/TextureProxy.h"

#include "Common/Caching/LocalFileCache.h"
#include "Common/Caching/CacheKey.h"

#include "Common/Files/GenericAssetFile.h"

#include "Misc/Paths.h"
#include "Engine/Texture2DArray.h"
#include "Misc/Crc.h"

DEFINE_LOG_CATEGORY(LogCityGMLBuildingMeshGenerator);

FCityGMLBuildingMeshGenerator::FCityGMLBuildingMeshGenerator(float WallThreshold, const FString &BasePath, int32 AtlasSize)
	:	m_BasePath(BasePath)
	,	m_AtlasSize(AtlasSize)
	,	m_WallThreshold(WallThreshold)
{
}

FCityGMLBuildingMeshGenerator::~FCityGMLBuildingMeshGenerator()
{

}


void FCityGMLBuildingMeshGenerator::Build(const TArray<GenericTwin::SCityGMLBuilding> &Buildings)
{
	BuildTextureAtlases(Buildings);

	for(const GenericTwin::SCityGMLBuilding &building : Buildings)
	{
		for(const GenericTwin::SCityGMLSurface &surface : building.surfaces)
		{
			if(m_AtlasMap.Contains(surface.texture_url))
			{
				AddTexturedSurface(surface);
			}
			else
			{
				AddUntexturedSurface(surface);
			}
		}
	}

}

void FCityGMLBuildingMeshGenerator::CompressTextures(bool WithAlphaChannel)
{
	UE_LOG(LogCityGMLBuildingMeshGenerator, Log, TEXT("Compressing atlas images") );
	for(const TextureAtlasPtr &texAtlasPtr : m_Atlases)
	{
		texAtlasPtr->Compress(WithAlphaChannel);
	}
	UE_LOG(LogCityGMLBuildingMeshGenerator, Log, TEXT("Done") );
}

GenericTwin::TextureProxyPtr FCityGMLBuildingMeshGenerator::GetAtlasTextureArray() const
{
	if(m_NumCachedAtlases)
	{
		GenericTwin::TextureProxyPtr atlasProxy = UGenericTwinTextureCache::GetInstance().CreateTextureArray(m_CachedAtlasSize, m_CachedAtlasSize, m_NumCachedAtlases, m_CachedPixelFormat);

		if	(	atlasProxy
			&&	atlasProxy->IsValid()
			)
		{
			uint8 *mipData = reinterpret_cast<uint8*>(atlasProxy->GetTexture2DArray()->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE));
			FMemory::Memcpy(mipData, m_CachedTextureData, m_CachedAtlasDataSize);

			atlasProxy->GetTexture2DArray()->GetPlatformData()->Mips[0].BulkData.Unlock();
			atlasProxy->GetTexture2DArray()->UpdateResource();
		}
		return atlasProxy;
	}
	else if(m_Atlases.IsEmpty() == false)
	{
		GenericTwin::TextureProxyPtr atlasProxy = UGenericTwinTextureCache::GetInstance().CreateTextureArray(m_AtlasSize, m_AtlasSize, m_Atlases.Num(), m_Atlases[0]->GetPixelFormat());

		if	(	atlasProxy
 			&&	atlasProxy->IsValid()
			)
		{
			const int32 imgMemSize = m_Atlases[0]->GetTextureDataSize();
			uint8 *mipData = reinterpret_cast<uint8*>(atlasProxy->GetTexture2DArray()->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE));
			for (int32 index = 0; index < m_Atlases.Num(); index++)
			{
				const TextureAtlasPtr &texAtlasPtr = m_Atlases[index];
				FMemory::Memcpy(mipData, texAtlasPtr->GetTextureData(), imgMemSize);
				mipData += imgMemSize;
			}
			atlasProxy->GetTexture2DArray()->GetPlatformData()->Mips[0].BulkData.Unlock();
			atlasProxy->GetTexture2DArray()->UpdateResource();
		}
		return atlasProxy;
	}

	return GenericTwin::TextureProxyPtr();
}


void FCityGMLBuildingMeshGenerator::BuildTextureAtlases(const TArray<GenericTwin::SCityGMLBuilding> &Buildings)
{
	UE_LOG(LogCityGMLBuildingMeshGenerator, Log, TEXT("Scanning %d buildings for textures"), Buildings.Num());

	TSet<FString> texUrls;
	int64 totalArea = 0;
	TArray<STextureData> textures;
	GenericTwin::SImageImportConfiguration imgImportCfg;
	imgImportCfg.MaxImageSize = m_AtlasSize;
	for(const GenericTwin::SCityGMLBuilding &building : Buildings)
	{
		for(const GenericTwin::SCityGMLSurface &surface : building.surfaces)
		{
			if	(	surface.texture_url.IsEmpty() == false
				&&	texUrls.Contains(surface.texture_url) == false
				)
			{
				texUrls.Add(surface.texture_url);

				FString fullUrl = FPaths::Combine(m_BasePath, surface.texture_url);
				GenericTwin::SImagePtr texImgPtr = GenericTwin::ImageImporter::ImportImageFromFile(fullUrl, imgImportCfg);

				if(texImgPtr)
				{
					STextureData texData;
					texData.Width = texImgPtr->GetWidth();
					texData.Height = texImgPtr->GetHeight();
					texData.Area = static_cast<uint32> (texData.Width * texData.Height);
					texData.Url = surface.texture_url;
					textures.Add(texData);

					totalArea += texData.Area;
				}
			}
		}
	}

	const int32 estimatedAtlasCount = static_cast<int32> ((totalArea + (m_AtlasSize * m_AtlasSize - 1)) / (m_AtlasSize * m_AtlasSize));

	UE_LOG(LogCityGMLBuildingMeshGenerator, Log, TEXT("Found %d referenced images with a total area of %d pixels requiring at least %d atlases"), textures.Num(), totalArea, estimatedAtlasCount);
	UE_LOG(LogCityGMLBuildingMeshGenerator, Log, TEXT("Starting building atlases") );

	if(textures.Num())
	{
		textures.Sort([](const STextureData &a, const STextureData &b) { return a.Area > b.Area; });

		for(STextureData &texData : textures)
		{
			FString fullUrl = FPaths::Combine(m_BasePath, texData.Url);
			GenericTwin::SImagePtr texImgPtr = GenericTwin::ImageImporter::ImportImageFromFile(fullUrl, imgImportCfg);
			if(texImgPtr)
			{
				bool added = false;
				for(int32 texAtlasInd = 0; texAtlasInd < m_Atlases.Num(); ++texAtlasInd)
				{
					const TextureAtlasPtr &texAtlasPtr = m_Atlases[texAtlasInd];
					added = texAtlasPtr->AddTexture(texData.Url, texImgPtr);
					if(added)
					{
						m_AtlasMap.Add(texData.Url, texAtlasInd);
						break;
					}
				}
				if(!added)
				{
					TextureAtlasPtr texAtlasPtr(new GenericTwin::FTextureAtlas(m_AtlasSize, 2));
					if(texAtlasPtr)
					{
						if(texAtlasPtr->AddTexture(texData.Url, texImgPtr))
						{
							const int32 texAtlasInd = m_Atlases.Num();
							m_AtlasMap.Add(texData.Url, texAtlasInd);

						}
						m_Atlases.Add(texAtlasPtr);
					}
				}
			}
		}

		UE_LOG(LogCityGMLBuildingMeshGenerator, Log, TEXT("Created %d atlases"), m_Atlases.Num());
		for(const TextureAtlasPtr &texAtlasPtr : m_Atlases)
		{
			const float remaining = texAtlasPtr->GetRemainingArea() * 100.0f / static_cast<float> (m_AtlasSize * m_AtlasSize);
			UE_LOG(LogCityGMLBuildingMeshGenerator, Log, TEXT("Coverage %4.1f"), 100.0f - remaining);
		}


#if WITH_EDITOR

		int32 num = 0;
		for(const TextureAtlasPtr &texAtlasPtr : m_Atlases)
		{
			FString fileName = FString(TEXT("e:/tmp/atlas_")) + FString::FromInt(num) + FString(TEXT(".bmp"));
			texAtlasPtr->GetAtlasImage()->SaveAsBMP(fileName);

			++num;
		}

#endif

	}
}

void FCityGMLBuildingMeshGenerator::AddTexturedSurface(const GenericTwin::SCityGMLSurface &Surface)
{
	const FString &texUrl = Surface.texture_url;
	const int32 texAtlasInd = m_AtlasMap[texUrl];
	const TextureAtlasPtr &texAtlasPtr = m_Atlases[texAtlasInd];

	GenericTwin::SMeshSection& meshSection = m_TexturedSection;

	FVector2D uv1(static_cast<double> (texAtlasInd), 0.0);

	const int32 startOffset = meshSection.Vertices.Num();
	for (const auto& vertex : Surface.vertices)
	{
		meshSection.Vertices.Add(vertex.position);
		meshSection.UV0.Add( texAtlasPtr->GetTextureCoordinate(texUrl, vertex.texture_coordinate) );
		meshSection.UV1.Add(uv1);
		meshSection.Normals.Add(vertex.normal);
	}

	for (int32 i = 0, count = Surface.indices.Num() / 3; i < count; ++i)
	{
		meshSection.Indices.Add(Surface.indices[3 * i] + startOffset);
		meshSection.Indices.Add(Surface.indices[3 * i + 1] + startOffset);
		meshSection.Indices.Add(Surface.indices[3 * i + 2] + startOffset);
	}

}

void FCityGMLBuildingMeshGenerator::AddUntexturedSurface(const GenericTwin::SCityGMLSurface &Surface)
{
	GenericTwin::SCityGMLSurface::Type surfaceType = Surface.surface_type;
	if(surfaceType == GenericTwin::SCityGMLSurface::Type::Unknown)
	{
		surfaceType = Surface.vertices[Surface.indices[0]].normal.Z <= m_WallThreshold ? GenericTwin::SCityGMLSurface::Type::Wall : GenericTwin::SCityGMLSurface::Type::Roof;
	}

	if	(	surfaceType == GenericTwin::SCityGMLSurface::Type::Wall
		||	surfaceType == GenericTwin::SCityGMLSurface::Type::Roof
		)
	{
		GenericTwin::SMeshSection& meshSection = surfaceType == GenericTwin::SCityGMLSurface::Type::Wall ? m_WallSection : m_RoofSection;

		const int32 startOffset = meshSection.Vertices.Num();
		for (const auto& vertex : Surface.vertices)
		{
			meshSection.Vertices.Add(vertex.position);
			meshSection.UV0.Add(vertex.texture_coordinate);
			meshSection.Normals.Add(vertex.normal);
		}

		for (int32 i = 0, count = Surface.indices.Num() / 3; i < count; ++i)
		{
			meshSection.Indices.Add(Surface.indices[3 * i] + startOffset);
			meshSection.Indices.Add(Surface.indices[3 * i + 1] + startOffset);
			meshSection.Indices.Add(Surface.indices[3 * i + 2] + startOffset);
		}
	}
}

void FCityGMLBuildingMeshGenerator::SaveToCache(const FString &Url, const FVector &LowerLeft)
{
	GenericTwin::FGenericAssetFile assetFile;

	if(assetFile.BeginCacheFile(Url, TEXT("citygml")))
	{
		if(m_Atlases.Num())
		{

			assetFile.AddCustomData(1, &LowerLeft, sizeof(FVector));

			uint32 totalSize = 0;
			for(auto texPtr : m_Atlases)
			{
				totalSize += texPtr->GetTextureDataSize();
			}

			assetFile.BeginTexture2D(static_cast<uint32> (m_AtlasSize), static_cast<uint32> (m_AtlasSize), static_cast<uint32> (m_Atlases.Num()), static_cast<uint32> (m_Atlases[0]->GetPixelFormat()), totalSize);
			for(const TextureAtlasPtr &texAtlasPtr : m_Atlases)
			{
				assetFile.AddToTexture2D(texAtlasPtr->GetTextureData(), texAtlasPtr->GetTextureDataSize());	
			}

			assetFile.AddMeshSection(0, m_TexturedSection);
			assetFile.AddMeshSection(0, m_RoofSection);
			assetFile.AddMeshSection(0, m_WallSection);
		}
	}
}

bool FCityGMLBuildingMeshGenerator::LoadFromCache(const FString &Url, FVector &LowerLeft)
{
	bool loadedFromCache = false;

	GenericTwin::GenericAssetFilePtr assetFile(new GenericTwin::FGenericAssetFile);

	if	(	assetFile
		&&	assetFile->LoadFromCache(Url, TEXT("citygml"))
		)
	{

		if	(	assetFile->GetNumMeshSections() == 3
			&&	assetFile->GetNumCustomDataChunks() == 1
			)
		{
			if	(	assetFile->GetNumTextures2D() > 0
				&&	assetFile->IsTexture2DArray(0)
				)
			{
				uint32 pixelFormat = 0;
				m_CachedTextureData = assetFile->GetTexture2DArray(0, m_CachedAtlasSize, m_CachedAtlasSize, m_NumCachedAtlases, pixelFormat, m_CachedAtlasDataSize);
				m_CachedPixelFormat = static_cast<EPixelFormat> (pixelFormat);

			}
			m_CachedAssetFile = assetFile;

			m_CachedAssetFile->GetMeshSection(0, m_TexturedSection);
			m_CachedAssetFile->GetMeshSection(1, m_RoofSection);
			m_CachedAssetFile->GetMeshSection(2, m_WallSection);
			loadedFromCache = true;

			LowerLeft = *( reinterpret_cast<const FVector*> (assetFile->GetCustomDataChunk(0)) );
		}
	}

	return loadedFromCache;
}
