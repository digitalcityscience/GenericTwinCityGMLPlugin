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

#include "Streaming/TileStreamingLayer.h"

#include "StreamingCityGMLLayer.generated.h"

namespace GenericTwin {

class ILocationMapper;
struct SCityGMLBuilding;

}	//	namespace GenericTwin 

class UMaterialInstanceDynamic;
class AGeoReferencingSystem;
class FCityGMLBuildingMeshGenerator;
class UStreamingCityGMLTile;

UCLASS()
class UStreamingCityGMLLayer	:	public UTileStreamingLayer
{
	GENERATED_BODY()

private:

	typedef TObjectPtr<UStreamingCityGMLTile> CityGMLTilePtr;

	class LoadModelJob	:	public GenericTwin::JobBase
	{
	public:

		LoadModelJob(UStreamingCityGMLLayer &Layer, AGeoReferencingSystem &GeoReferencingSystem, const FString &FileName, const CityGMLTilePtr &TilePtr);

		virtual GenericTwin::TJobBasePtr Execute() override;

		virtual void OnJobFinished() override;

	private:

		UStreamingCityGMLLayer 						&m_Layer;
		AGeoReferencingSystem						&m_GeoReferencingSystem;
		FString										m_FileName;

		CityGMLTilePtr								m_TilePtr;

		TSharedPtr<FCityGMLBuildingMeshGenerator>	m_MeshGeneratorPtr;
		FVector										m_LowerLeft;

	};

public:

	static const SLayerMetaData& GetLayerMetaData();
	static TSharedPtr<FJsonObject> GetDefaultConfiguration();
	static ULayerBase* Create(FTwinWorldContext &TwinWorldCtx, const TSharedPtr<FJsonObject> &Parameters);

	UStreamingCityGMLLayer();

	virtual ~UStreamingCityGMLLayer();

	virtual void InitializeLayer(FTwinWorldContext &TwinWorldCtx) override;

	virtual void UpdateView(FTwinWorldContext &TwinWorldCtx, float DeltaSeconds) override;

	virtual void OnEndPlay(FTwinWorldContext &TwinWorldCtx) override;

	virtual TileBasePtr CreateTile(FTwinWorldContext &TwinWorldCtx, const GenericTwin::TTileId &TileId) override;

	virtual const TSharedPtr<FJsonObject> GetRuntimeConfiguration() const override;
	virtual void UpdateConfiguration(TSharedPtr<FJsonObject> UpdatedConfiguration) override;

	bool LoadCityGMLTile(const GenericTwin::TTileId &TileId);

private:

	void ExtractParameters(const FJsonObject &Parameters, FTwinWorldContext &TwinWorldCtx);
	void ExtractModelParameters(const FJsonObject &ParametersObject);

	GenericTwin::TTileId MapLocationToTileId(const FVector& Location);

	void CreateMaterials();

	void FillFeatureCache(const TArray<GenericTwin::SCityGMLBuilding> &Buildings, AGeoReferencingSystem	&GeoRefSystem, const FVector &LowerLeft);

	GenericTwin::ILocationMapper			*m_LocationMapper = 0;

	FString									m_StreamingConfigFile;
	TSharedPtr<GenericTwin::ITileIndex>		m_TileIndex;

	AGeoReferencingSystem					*m_GeoReferencingSystem = 0;

	bool									m_DisableCaching = false;

	FString									m_Theme;

	FLinearColor							m_WallAlbedo = FLinearColor(0.2, 0.2, 0.2, 1.0);
	FLinearColor							m_RoofAlbedo = FLinearColor(0.1, 0.1, 0.1, 1.0);

	bool									m_FillFeatureCache = false;
	float									m_HeightOffset = 0.0f;

	UPROPERTY()
	UMaterialInterface						*m_TexturedBaseMaterial = 0;

	UPROPERTY()
	UMaterialInstanceDynamic				*m_WallMaterial = 0;

	UPROPERTY()
	UMaterialInstanceDynamic				*m_RoofMaterial = 0;

};

