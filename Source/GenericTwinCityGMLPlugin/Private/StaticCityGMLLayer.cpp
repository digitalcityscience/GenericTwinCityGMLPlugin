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


#include "StaticCityGMLLayer.h"

#include "TwinWorldContext.h"
#include "Common/Configuration/ConfigurationJsonBuilder.h"

#include "GenericTwinCityGMLImport.h"
#include "ICityGMLCoordinateTransform.h"

#include "CityGMLBuildingMeshGenerator.h"

#include "Utils/PathUtils.h"

#include "GeoReferencingSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"

#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#include "Common/JobQueue/GenericTwinJobDispatcher.h"
#include "Actors/GenericTwinCustomMeshActor.h"

const SLayerMetaData& UStaticCityGMLLayer::GetLayerMetaData()
{
	static SLayerMetaData layerMetaData =	{	TEXT("StaticCityGML")
											,	FText::FromString(TEXT("Static City GML"))
											,	FText::FromString(TEXT("Provides static CityGML geometry"))
											,	ELayerContributionType::Visualisation
											,	ELayerCreationType::RuntimeOnce
											};

	return layerMetaData;
}

TSharedPtr<FJsonObject> UStaticCityGMLLayer::GetDefaultConfiguration()
{
	static TSharedPtr<FJsonObject> defaultConfig;

	if(!defaultConfig)
	{
		ConfigurationJsonBuilder jcb;
		jcb.Begin();
		jcb.Add(TEXT("base_path"), TEXT("string"), TEXT("Verzeichnis"), false);
		jcb.Add(TEXT("files"), TEXT("string"), TEXT("CityGML Dateien"), false);
		jcb.AddLiteralValue(TEXT("roof_albedo"), TEXT("color"), TEXT("Dachfarbe"), false, TEXT("[0.8,0.3,0.0]"));
		jcb.AddLiteralValue(TEXT("wall_albedo"), TEXT("color"), TEXT("Wandfarbe"), false, TEXT("[0.4,0.4,0.4]"));
		defaultConfig = jcb.Finalize();
	}

	return defaultConfig;
}

ULayerBase* UStaticCityGMLLayer::Create(FTwinWorldContext &TwinWorldCtx, const FJsonObject &Parameters)
{
	UStaticCityGMLLayer *layer = NewObject<UStaticCityGMLLayer>(TwinWorldCtx.World, FName(), RF_Standalone);

	if(layer)
	{
		layer->OnConstruction(GetLayerMetaData(), ELayerPriority::Medium);
		layer->extractParameters(Parameters);
	}

	return layer;
}


UStaticCityGMLLayer::UStaticCityGMLLayer()
{
}

const TSharedPtr<FJsonObject> UStaticCityGMLLayer::GetRuntimeConfiguration() const
{
	const FString jsonFormatStr = FString(TEXT("{\"configuration\":[{\"key\":\"wall_albedo\",\"type\":\"color\",\"display_name\":\"Wandfarbe\",\"is_realtime\":true,\"value\": [{0},{1},{2}]},{\"key\":\"roof_albedo\",\"type\":\"color\",\"display_name\":\"Dachfarbe\",\"is_realtime\":true,\"value\": [{3},{4},{5}]}]}"));
	const FString jsonStr = FString::Format(*jsonFormatStr, {m_WallAlbedo.R, m_WallAlbedo.G, m_WallAlbedo.B, m_RoofAlbedo.R, m_RoofAlbedo.G, m_RoofAlbedo.B });

	TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(jsonStr);
	TSharedPtr<FJsonObject> jsonObject;
	FJsonSerializer::Deserialize(reader, jsonObject);

	return jsonObject;
}

void UStaticCityGMLLayer::UpdateConfiguration(TSharedPtr<FJsonObject> UpdatedConfiguration)
{
	if(UpdatedConfiguration->HasTypedField<EJson::Array>(TEXT("wall_albedo")))
	{
		const TArray<TSharedPtr<FJsonValue>> arrayVals = UpdatedConfiguration->GetArrayField(TEXT("wall_albedo"));
		m_WallAlbedo = FLinearColor(arrayVals[0]->AsNumber(), arrayVals[1]->AsNumber(), arrayVals[2]->AsNumber(), 1.0f);
		if(m_WallMaterial)
		{
			m_WallMaterial->SetVectorParameterValue("Albedo", m_WallAlbedo);
		}
	}
	if (UpdatedConfiguration->HasTypedField<EJson::Array>(TEXT("roof_albedo")))
	{
		const TArray<TSharedPtr<FJsonValue>> arrayVals = UpdatedConfiguration->GetArrayField(TEXT("roof_albedo"));
		m_RoofAlbedo = FLinearColor(arrayVals[0]->AsNumber(), arrayVals[1]->AsNumber(), arrayVals[2]->AsNumber(), 1.0f);
		if (m_RoofMaterial)
		{
			m_RoofMaterial->SetVectorParameterValue("Albedo", m_RoofAlbedo);
		}
	}
}

void UStaticCityGMLLayer::extractParameters(const FJsonObject &Parameters)
{
	if	(	Parameters.HasTypedField<EJson::String>(TEXT("base_path"))
		&&	Parameters.HasTypedField<EJson::String>(TEXT("files"))
		)
	{
		m_BasePath = Parameters.GetStringField("base_path");
		FString files = Parameters.GetStringField("files");
		TArray<FString> parts;
		files.ParseIntoArray(parts, TEXT(","));
		for(const auto &p : parts)
			m_Files.Add( p.TrimStartAndEnd() );

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

		if (Parameters.HasTypedField<EJson::Boolean>(TEXT("disable_caching")))
			m_DisableCaching = Parameters.GetBoolField(TEXT("disable_caching"));

		// const TArray < TSharedPtr < FJsonValue > > *filesArray = 0;
		// if	(	Parameters.TryGetArrayField(TEXT("files"), filesArray)
		// 	&&	filesArray
		// 	)
		// {
		// 	for(const auto &fileVal : (*filesArray))
		// 	{
		// 		m_Files.Add( fileVal->AsString() );
		// 	}
		// }

	}
}

UStaticCityGMLLayer::~UStaticCityGMLLayer()
{
}

void UStaticCityGMLLayer::InitializeLayer(FTwinWorldContext &TwinWorldCtx)
{
	createMaterials(TwinWorldCtx.World);
	m_nextFileToLoad = 0;
	m_isFilePending = false;
}

void UStaticCityGMLLayer::UpdateView(FTwinWorldContext &TwinWorldCtx, float DeltaSeconds)
{
	if	(	!m_isFilePending
		&&	m_nextFileToLoad < m_Files.Num()
		)
	{
		const FString fileName = FPaths::Combine(m_BasePath, m_Files[m_nextFileToLoad]);
		TSharedPtr<LoadModelJob> loadJobPtr(new LoadModelJob(*this, *(TwinWorldCtx.GeoReferencingSystem), GenericTwin::FPathUtils::MakeAbsolutPath(fileName, FPaths::LaunchDir())));

		FGenericTwinJobDispatcher::DispatchJob(loadJobPtr);

		m_isFilePending = true;
	}
}

void UStaticCityGMLLayer::OnVisibilityChanged(FTwinWorldContext& TwinWorldCtx)
{
	for(AActor *actor : m_Actors)
		actor->SetActorHiddenInGame(!m_isVisible);
}

void UStaticCityGMLLayer::OnEndPlay(FTwinWorldContext &TwinWorldCtx)
{
	for(AGenericTwinCustomMeshActor* a : m_Actors)
	{
		a->Release();
	}
}

UStaticCityGMLLayer::LoadModelJob::LoadModelJob(UStaticCityGMLLayer &Layer, AGeoReferencingSystem &GeoReferencingSystem, const FString &FileName)
	:	JobBase(EJobType::Background)
	,	m_Layer(Layer)
	,	m_GeoReferencingSystem(GeoReferencingSystem)
	,	m_FileName(FileName)
{
}

GenericTwin::TJobBasePtr UStaticCityGMLLayer::LoadModelJob::Execute()
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


void UStaticCityGMLLayer::LoadModelJob::OnJobFinished()
{
	AGenericTwinCustomMeshActor* actor = 0;
	
	actor = m_Layer.GetWorld()->SpawnActor<AGenericTwinCustomMeshActor>(FVector::ZeroVector, FRotator::ZeroRotator, FActorSpawnParameters());
	if(actor)
	{
		if(m_MeshGeneratorPtr->GetTexturedMeshSection().IsEmpty() == false)
		{
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
			actor->AddMeshSection(m_MeshGeneratorPtr->GetTexturedMeshSection(), materialInst);
		}

		actor->AddMeshSection(m_MeshGeneratorPtr->GetRoofMeshSection(), m_Layer.m_RoofMaterial);
		actor->AddMeshSection(m_MeshGeneratorPtr->GetWallMeshSection(), m_Layer.m_WallMaterial);

		actor->SetActorHiddenInGame(!m_Layer.m_isVisible);
		actor->SetActorLocation(m_LowerLeft);

		m_Layer.m_Actors.Add(actor);
	}

	m_Layer.m_isFilePending = false;
	m_Layer.m_nextFileToLoad++;
}


void UStaticCityGMLLayer::createMaterials(UWorld *World)
{
	UMaterial *texBaseMat = Cast<UMaterial>(StaticLoadObject(UMaterial::StaticClass(), 0, TEXT("/GenericTwinCityGMLPlugin/Materials/M_CityGML_Textured")));
	if(texBaseMat)
	{
		m_TexturedBaseMaterial = texBaseMat;
	}

	UMaterialInstance*roofBaseMat = Cast<UMaterialInstance>(StaticLoadObject(UMaterialInstance::StaticClass(), 0, TEXT("/GenericTwinCityGMLPlugin/Materials/MI_CityGML_Roof")));
	if(roofBaseMat)
	{
		m_RoofMaterial = UMaterialInstanceDynamic::Create(roofBaseMat, World);
		if(m_RoofMaterial)
		{
			m_RoofMaterial->SetVectorParameterValue("Albedo", m_RoofAlbedo);
		}
	}

	UMaterialInstance*wallBaseMat = Cast<UMaterialInstance>(StaticLoadObject(UMaterialInstance::StaticClass(), 0, TEXT("/GenericTwinCityGMLPlugin/Materials/MI_CityGML_Wall")));
	if(wallBaseMat)
	{
		m_WallMaterial = UMaterialInstanceDynamic::Create(wallBaseMat, World);
		if(m_WallMaterial)
		{
			m_WallMaterial->SetVectorParameterValue("Albedo", m_WallAlbedo);
		}
	}
}
