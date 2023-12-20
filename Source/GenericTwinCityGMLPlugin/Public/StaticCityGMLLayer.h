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

#include "Layers/LayerBase.h"
#include "Layers/LayerTypes.h"

#include "Common/JobQueue/GenericTwinJobBase.h"
#include "Common/BaseTypes.h"

#include "StaticCityGMLLayer.generated.h" 

class UMaterialInstanceDynamic;
class AGeoReferencingSystem;
class AGenericTwinCustomMeshActor;

class FCityGMLBuildingMeshGenerator;

UCLASS()
class UStaticCityGMLLayer	:	public ULayerBase
{
	GENERATED_BODY()

private:

	class LoadModelJob	:	public GenericTwin::JobBase
	{
	public:

		LoadModelJob(UStaticCityGMLLayer &Layer, AGeoReferencingSystem &GeoReferencingSystem, const FString &FileName);

		virtual GenericTwin::TJobBasePtr Execute() override;

		virtual void OnJobFinished() override;

	private:

		UStaticCityGMLLayer 						&m_Layer;
		AGeoReferencingSystem						&m_GeoReferencingSystem;
		FString										m_FileName;

		TSharedPtr<FCityGMLBuildingMeshGenerator>	m_MeshGeneratorPtr;
		FVector										m_LowerLeft;

	};

public:

	static const SLayerMetaData& GetLayerMetaData();
	static TSharedPtr<FJsonObject> GetDefaultConfiguration();
	static ULayerBase* Create(FTwinWorldContext &TwinWorldCtx, const FJsonObject &Parameters);

	virtual ~UStaticCityGMLLayer();

	virtual void InitializeLayer(FTwinWorldContext &TwinWorldCtx) override;

	virtual void UpdateView(FTwinWorldContext &TwinWorldCtx, float DeltaSeconds) override;

	virtual void OnVisibilityChanged(FTwinWorldContext& TwinWorldCtx) override;

	virtual void OnEndPlay(FTwinWorldContext& TwinWorldCtx) override;

	virtual const TSharedPtr<FJsonObject> GetRuntimeConfiguration() const override;
	virtual void UpdateConfiguration(TSharedPtr<FJsonObject> UpdatedConfiguration);

private:

	UStaticCityGMLLayer();

	void extractParameters(const FJsonObject &Parameters);

	void createMaterials(UWorld *World);

	FString										m_BasePath;
	TArray<FString>								m_Files;
	int32										m_nextFileToLoad = 0;
	bool										m_isFilePending = false;

	bool										m_DisableCaching = false;

	FLinearColor								m_WallAlbedo = FLinearColor(0.2, 0.2, 0.2, 1.0);
	FLinearColor								m_RoofAlbedo = FLinearColor(0.1, 0.1, 0.1, 1.0);;

	UPROPERTY()
	UMaterialInterface							*m_TexturedBaseMaterial = 0;

	UPROPERTY()
	UMaterialInstanceDynamic					*m_WallMaterial = 0;

	UPROPERTY()
	UMaterialInstanceDynamic					*m_RoofMaterial = 0;

	TArray<AGenericTwinCustomMeshActor*>		m_Actors;

};
