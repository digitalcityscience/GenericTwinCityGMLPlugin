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

#include "StreamingCityGMLLayer.h"
#include "TwinWorldContext.h"
#include "Common/Configuration/ConfigurationJsonBuilder.h"
#include "StreamingCityGMLTile.h"

#include "Streaming/TileIndexes/FileSystemTileIndex.h"
#include "Utils/FilePathUtils.h"
#include "Actors/GenericTwinStaticMeshActor.h"

#include "ICityGMLImporter.h"

#include "ICityGMLProvider.h"
#include "CityGMLProviders/CityGMLTileProvider.h"

#include "SpatialPartitioning/TileManager/TilingHelpers.h"

const SLayerMetaData& UStreamingCityGMLLayer::GetLayerMetaData()
{
	static SLayerMetaData layerMetaData =	{	TEXT("StreamingCityGML")
											,	FText::FromString(TEXT("Streaming City GML"))
											,	FText::FromString(TEXT("Streams CityGML geometry at runtime"))
											,	ELayerContributionType::Visualisation
											,	ELayerCreationType::RuntimeOnce
											};

	return layerMetaData;
}

TSharedPtr<FJsonObject> UStreamingCityGMLLayer::GetDefaultConfiguration()
{
	static TSharedPtr<FJsonObject> defaultConfig;

	if(!defaultConfig)
	{
		ConfigurationJsonBuilder jcb;
		jcb.Begin();
		jcb.Add(TEXT("base_path"), TEXT("string"), TEXT("Verzeichnis"), false);
		jcb.AddLiteralValue(TEXT("roof_albedo"), TEXT("color"), TEXT("Dachfarbe"), false, TEXT("[0.8,0.3,0.0]"));
		jcb.AddLiteralValue(TEXT("wall_albedo"), TEXT("color"), TEXT("Wandfarbe"), false, TEXT("[0.4,0.4,0.4]"));
		defaultConfig = jcb.Finalize();
	}

	return defaultConfig;
}

ULayerBase* UStreamingCityGMLLayer::Create(FTwinWorldContext &TwinWorldCtx, const TSharedPtr<FJsonObject> &Parameters)
{
	UStreamingCityGMLLayer *layer = NewObject<UStreamingCityGMLLayer>(TwinWorldCtx.World, FName(), RF_Standalone);

	if(layer)
	{
		layer->OnConstruction(GetLayerMetaData(), ELayerPriority::Medium);
		layer->ExtractParameters(*Parameters, TwinWorldCtx);
	}

	return layer;
}


UStreamingCityGMLLayer::UStreamingCityGMLLayer()
{
}

UStreamingCityGMLLayer::~UStreamingCityGMLLayer()
{
}

void UStreamingCityGMLLayer::ExtractParameters(const FJsonObject &Parameters, FTwinWorldContext &TwinWorldCtx)
{
	if(Parameters.HasTypedField<EJson::String>(TEXT("streaming_description")))
	{
		m_StreamingConfigFile = Parameters.GetStringField("streaming_description");

		m_DrawTileBorders &= Parameters.TryGetBoolField(TEXT("draw_tile_borders"), m_DrawTileBorders);

		if (Parameters.HasTypedField<EJson::String>(TEXT("theme")))
			m_Theme = Parameters.GetStringField("theme");

		if (Parameters.HasTypedField<EJson::Array>(TEXT("wall_albedo")))
		{
			const TArray<TSharedPtr<FJsonValue>> arrayVals = Parameters.GetArrayField(TEXT("wall_albedo"));
			m_WallAlbedo = FLinearColor(arrayVals[0]->AsNumber(), arrayVals[1]->AsNumber(), arrayVals[2]->AsNumber(), 1.0f);
		}

		if (Parameters.HasTypedField<EJson::Array>(TEXT("roof_albedo")))
		{
			const TArray<TSharedPtr<FJsonValue>> arrayVals = Parameters.GetArrayField(TEXT("roof_albedo"));
			m_RoofAlbedo = FLinearColor(arrayVals[0]->AsNumber(), arrayVals[1]->AsNumber(), arrayVals[2]->AsNumber(), 1.0f);
		}

		if (Parameters.HasTypedField<EJson::Number>(TEXT("height_offset_m")))
			m_HeightOffset = static_cast<float>(Parameters.GetNumberField(TEXT("height_offset_m"))) * 100.0f;

		if (Parameters.HasTypedField<EJson::Boolean>(TEXT("disable_caching")))
			m_DisableCaching = Parameters.GetBoolField(TEXT("disable_caching"));

		UTileManager::SConfiguration config;
		config.MaxViewDistance = (Parameters.HasTypedField<EJson::Number>(TEXT("view_distance")) ? Parameters.GetNumberField(TEXT("view_distance")) : 4000.0f) * 100.0;
		config.MaxLoadDistance = Parameters.HasTypedField<EJson::Number>(TEXT("load_distance")) ? (Parameters.GetNumberField(TEXT("load_distance")) * 100.0) : config.MaxViewDistance;
		config.ViewRectHalfSize = Parameters.HasTypedField<EJson::Number>(TEXT("view_rect_half_size")) ? Parameters.GetNumberField(TEXT("view_rect_half_size")) : 3;
		config.MaxHeapMemoroyUsage = (Parameters.HasTypedField<EJson::Number>(TEXT("max_heap_memory_mb")) ? Parameters.GetNumberField(TEXT("max_heap_memory_mb")) : 100) * 1024 * 1024;
		config.MaxGPUMemoryUsage = (Parameters.HasTypedField<EJson::Number>(TEXT("max_gpu_memory_mb")) ? Parameters.GetNumberField(TEXT("max_gpu_memory_mb")) : 250) * 1024 * 1024;

		ConfigureTileManager(TEXT("CityGML"), config);
	}
}

void UStreamingCityGMLLayer::ExtractModelParameters(const FJsonObject &ParametersObject)
{
}

void UStreamingCityGMLLayer::InitializeLayer(FTwinWorldContext &TwinWorldCtx)
{
	class InitTileIndexJob	:	public GenericTwin::JobBase
	{
	public:

		InitTileIndexJob(UStreamingCityGMLLayer &Layer, AGeoReferencingSystem &GeoReferencingSystem, const FString &FileName)
			:	JobBase(GenericTwin::EJobType::Background)
			,	m_Layer(Layer)
			,	m_GeoReferencingSystem(GeoReferencingSystem)
			,	m_FileName(FileName)
		{
		}

		virtual GenericTwin::TJobBasePtr Execute() override
		{
			GenericTwin::FileSystemTileIndex *tileIndex = new GenericTwin::FileSystemTileIndex(m_GeoReferencingSystem);
			if(tileIndex)
			{
				if(tileIndex->Initialize(GenericTwin::FPathUtils::MakeAbsolutPath(m_FileName, FPaths::LaunchDir()), std::bind(&UStreamingCityGMLLayer::ExtractModelParameters, &m_Layer, std::placeholders::_1)))
				{
					m_TileIndex = TSharedPtr<GenericTwin::ITileIndex>(tileIndex);
				}
			}
			return GenericTwin::TJobBasePtr();
		}

		virtual void OnJobFinished() override
		{
			if(m_TileIndex)
			{
				m_Layer.m_TileIndex = m_TileIndex;
				m_Layer.m_LocationMapper = new GenericTwin::BasicLocationMapper(m_Layer.m_TileIndex->GetReferenceLocation(), m_Layer.m_TileIndex->GetTileSize());
				if(m_Layer.m_LocationMapper)
				{
					m_Layer.SetupTiling(*(m_Layer.m_LocationMapper), 5);
				}
			}
		}

	private:

		UStreamingCityGMLLayer					&m_Layer;
		AGeoReferencingSystem					&m_GeoReferencingSystem;
		FString									m_FileName;
		TSharedPtr<GenericTwin::ITileIndex>		m_TileIndex;
	};

	TSharedPtr<InitTileIndexJob> initJobPtr(new InitTileIndexJob(*this, *(TwinWorldCtx.GeoReferencingSystem), m_StreamingConfigFile));
	FGenericTwinJobDispatcher::DispatchJob(initJobPtr);

	CreateMaterials();

	m_GeoReferencingSystem = TwinWorldCtx.GeoReferencingSystem;
}

void UStreamingCityGMLLayer::UpdateView(FTwinWorldContext &TwinWorldCtx, float DeltaSeconds)
{
	Super::UpdateView(TwinWorldCtx, DeltaSeconds);

}

void UStreamingCityGMLLayer::OnEndPlay(FTwinWorldContext &TwinWorldCtx)
{
	Super::OnEndPlay(TwinWorldCtx);
}

TileBasePtr UStreamingCityGMLLayer::CreateTile(FTwinWorldContext &TwinWorldCtx, const GenericTwin::TTileId &TileId)
{
	UStreamingCityGMLTile *tile = 0;

	if(m_TileIndex && m_TileIndex->HasTile(TileId))
	{
		tile = NewObject<UStreamingCityGMLTile> (TwinWorldCtx.World, FName(), RF_Standalone);

		if(tile)
		{
			tile->SetLayer(this);
			tile->SetTileId(TileId);

			FVector location = TileId.ToLocation(m_TileIndex->GetReferenceLocation(), m_TileIndex->GetTileSize());
			tile->Initialize(location, m_TileIndex->GetTileSize());
		}
	}

	return TileBasePtr(tile);
}

const TSharedPtr<FJsonObject> UStreamingCityGMLLayer::GetRuntimeConfiguration() const
{
	ConfigurationJsonBuilder cfgBuilder;

	cfgBuilder.Begin();

	cfgBuilder.AddLiteralValue(TEXT("height_offset_m"), TEXT("number"), TEXT("Höhenoffset in m"), true, FString::SanitizeFloat(m_HeightOffset));

	return cfgBuilder.Finalize();
}

void UStreamingCityGMLLayer::UpdateConfiguration(TSharedPtr<FJsonObject> UpdatedConfiguration)
{
	if(UpdatedConfiguration->HasTypedField<EJson::Number>(TEXT("height_offset_m")))
	{
		m_HeightOffset = UpdatedConfiguration->GetNumberField(TEXT("height_offset_m")) * 100.0;
		
		const float heightOffset = m_HeightOffset;
		FProcessTileFunction updateHO;
		updateHO.BindLambda([heightOffset](UTileBase& Tile) {
			UStreamingCityGMLTile* gmlTile = Cast<UStreamingCityGMLTile>(&Tile);
			if (gmlTile)
			{
				gmlTile->UpdatedHeightOffset(heightOffset);
			}
			});
		ForEachTile(updateHO);
	}
}

bool UStreamingCityGMLLayer::LoadCityGMLTile(const GenericTwin::TTileId &TileId)
{
	TileBasePtr tile = GetTile(TileId);
	if	(	tile
		&&	FGenericTwinCityGMLImportModule::HasImporter()
		)
	{
		TSharedPtr<LoadModelJob> loadJobPtr(new LoadModelJob(*this, *m_GeoReferencingSystem, m_TileIndex->GetUrl(TileId), CityGMLTilePtr( static_cast<UStreamingCityGMLTile*> (tile.Get()) )));
		FGenericTwinJobDispatcher::DispatchJob(loadJobPtr);
		return true;
	}

	return false;
}


void UStreamingCityGMLLayer::CreateMaterials()
{
	UMaterial *texBaseMat = Cast<UMaterial>(StaticLoadObject(UMaterial::StaticClass(), 0, TEXT("/GenericTwinCityGMLPlugin/Materials/M_CityGML_Textured")));
	if(texBaseMat)
	{
		m_TexturedBaseMaterial = texBaseMat;
	}

	UMaterialInstance *roofBaseMat = Cast<UMaterialInstance>(StaticLoadObject(UMaterialInstance::StaticClass(), 0, TEXT("/GenericTwinCityGMLPlugin/Materials/MI_CityGML_Roof")));
	if(roofBaseMat)
	{
		m_RoofMaterial = UMaterialInstanceDynamic::Create(roofBaseMat, GetWorld());
		if(m_RoofMaterial)
		{
			m_RoofMaterial->SetVectorParameterValue("Albedo", m_RoofAlbedo);
		}
	}

	UMaterialInstance *wallBaseMat = Cast<UMaterialInstance>(StaticLoadObject(UMaterialInstance::StaticClass(), 0, TEXT("/GenericTwinCityGMLPlugin/Materials/MI_CityGML_Wall")));
	if(wallBaseMat)
	{
		m_WallMaterial = UMaterialInstanceDynamic::Create(wallBaseMat, GetWorld());
		if(m_WallMaterial)
		{
			m_WallMaterial->SetVectorParameterValue("Albedo", m_WallAlbedo);
		}
	}
}



UStreamingCityGMLLayer::LoadModelJob::LoadModelJob(UStreamingCityGMLLayer&Layer, AGeoReferencingSystem &GeoReferencingSystem, const FString &FileName, const CityGMLTilePtr &TilePtr)
	:	JobBase(GenericTwin::EJobType::Background)
	,	m_Layer(Layer)
	,	m_GeoReferencingSystem(GeoReferencingSystem)
	,	m_FileName(FileName)
	,	m_TilePtr(TilePtr)
{
}

GenericTwin::TJobBasePtr UStreamingCityGMLLayer::LoadModelJob::Execute()
{
	class Transform	: public GenericTwin::ICityGMLCoordinateTransform
	{
	public:

		Transform(AGeoReferencingSystem &geoRefSystem, const FVector &refPointUE)
			:	m_GeoRefSystem(geoRefSystem)
			,	m_RefPointUE(refPointUE)
		{	}

		virtual FVector operator() (const FVector &pos) override
		{
			FVector transformed;
			m_GeoRefSystem.ProjectedToEngine(pos, transformed);
			transformed -= m_RefPointUE;
			return transformed;
		}

	private:

		AGeoReferencingSystem	&m_GeoRefSystem;
		FVector					m_RefPointUE;
		float					m_HeightOffset = 0.0f;
	};

	m_MeshGeneratorPtr = TSharedPtr<FCityGMLBuildingMeshGenerator>( new FCityGMLBuildingMeshGenerator(0.2f, FPaths::GetPath(m_FileName), 4096) );
	if(m_MeshGeneratorPtr)
	{
		if	(	m_Layer.m_DisableCaching
			||	m_MeshGeneratorPtr->LoadFromCache(m_FileName, m_LowerLeft) == false
			)
		{
			TSharedPtr<GenericTwin::ICityGMLImporter> importer = FGenericTwinCityGMLImportModule::GetImporter();
			if	(	importer
				&&	importer->Load(m_FileName)
				)
			{
				m_GeoReferencingSystem.ProjectedToEngine(importer->GetMinExtent(), m_LowerLeft);
				m_LowerLeft.Z = 0.0;

				Transform transform(m_GeoReferencingSystem, m_LowerLeft);

				TArray<GenericTwin::SCityGMLBuilding> buildings = importer->GetBuildings(transform, m_Layer.m_Theme);
				m_MeshGeneratorPtr->Build(buildings);
				m_MeshGeneratorPtr->CompressTextures(false);

				if(m_Layer.m_DisableCaching == false)
					m_MeshGeneratorPtr->SaveToCache(m_FileName, m_LowerLeft);
			}
		}
	}

	return GenericTwin::TJobBasePtr();
}

void UStreamingCityGMLLayer::LoadModelJob::OnJobFinished()
{
	if(m_MeshGeneratorPtr)
	{
		AGenericTwinStaticMeshActor* actor = 0;
		
		actor = m_Layer.GetWorld()->SpawnActor<AGenericTwinStaticMeshActor>(FVector::ZeroVector, FRotator::ZeroRotator, FActorSpawnParameters());
		if(actor)
		{
			actor->Initialize();
			const auto jobType = actor->GetUploadMode() == GenericTwin::IStaticMeshComponent::EUploadMode::GameThread ? GenericTwin::EJobType::GameThread : GenericTwin::EJobType::Background;

			TSharedPtr<FCityGMLBuildingMeshGenerator> meshGeneratorPtr = m_MeshGeneratorPtr;
			if(m_MeshGeneratorPtr->GetTexturedMeshSection().IsEmpty() == false)
			{
				// Todo: store material instance as UPROPERTY to prevent garbage collection
				UMaterialInstanceDynamic *materialInst = UMaterialInstanceDynamic::Create(m_Layer.m_TexturedBaseMaterial, m_Layer.GetWorld());
				if(materialInst)
				{
					GenericTwin::TextureProxyPtr texProxyPtr = m_MeshGeneratorPtr->GetAtlasTextureArray();
					if	(	texProxyPtr
						&&	texProxyPtr->IsValid()
						)
					{
						FString samplerName(TEXT("TextureAtlasArray"));
						materialInst->SetTextureParameterValue(*samplerName, texProxyPtr->GetTexture2DArray());
						actor->AddTextureProxy(texProxyPtr);
					}
				}
				// actor->AddMeshSection(m_MeshGeneratorPtr->GetTexturedMeshSection(), materialInst);
				FGenericTwinJobDispatcher::Dispatch<void()>([actor, meshGeneratorPtr, materialInst]() {	actor->AddMeshSection(meshGeneratorPtr->GetTexturedMeshSection(), materialInst); }, jobType);
			}

			UMaterialInstanceDynamic *roofMatInst = m_Layer.m_RoofMaterial;
			FGenericTwinJobDispatcher::Dispatch<void()>([actor, meshGeneratorPtr, roofMatInst]() {	actor->AddMeshSection(meshGeneratorPtr->GetRoofMeshSection(), roofMatInst); }, jobType);

			UMaterialInstanceDynamic *wallMatInst = m_Layer.m_WallMaterial;
			FGenericTwinJobDispatcher::Dispatch<void()>([actor, meshGeneratorPtr, wallMatInst]() {	actor->AddMeshSection(meshGeneratorPtr->GetWallMeshSection(), wallMatInst); }, jobType);

			// actor->AddMeshSection(m_MeshGeneratorPtr->GetRoofMeshSection(), m_Layer.m_RoofMaterial);
			// actor->AddMeshSection(m_MeshGeneratorPtr->GetWallMeshSection(), m_Layer.m_WallMaterial);

			actor->SetActorHiddenInGame(!m_Layer.m_isVisible);
			actor->SetActorLocation(FVector(m_LowerLeft.X, m_LowerLeft.Y, m_LowerLeft.Z + m_Layer.m_HeightOffset));	// m_Layer.m_TileIndex->GetTileOrigin(m_TilePtr->GetTileId()));

		}

		m_TilePtr->AddToWorld(actor);
	}
}
