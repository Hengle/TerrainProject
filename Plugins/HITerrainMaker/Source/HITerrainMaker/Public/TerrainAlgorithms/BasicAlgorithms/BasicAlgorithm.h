// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "TerrainAlgorithms/HITerrainAlgorithm.h"
#include "TerrainMaths/HITerrainCurvedPerlin.h"
#include "TerrainMaths/noiselib/noise.h"
#include "BasicAlgorithm.generated.h"

/**
 * 
 */
UCLASS()
class HITERRAINMAKER_API UBasicAlgorithm : public UHITerrainAlgorithm
{
	GENERATED_BODY()

public:
	virtual void Init(FTerrainInformationPtr InInformation) override;
	virtual void ApplyAlgorithm(UHITerrainData* Data) override;
	virtual void DebugAlgorithm(UHITerrainData* Data) override;

private:
	noise::module::Perlin BaseLandscape;
	noise::module::Curve BaseLandscape_Curve;
	noise::module::Perlin BaseLandscape2;
	noise::module::ScaleBias BaseLandscape2_ScaleBias;
	noise::module::Min BaseLandscape_Min;
	noise::module::Clamp BaseLandscape_Clamp;
	noise::module::Cache BaseLandscape_Cache;

	noise::module::Turbulence Landscape_Turbulence1;
	noise::module::Turbulence Landscape_Turbulence2;
	noise::module::Turbulence Landscape_Turbulence3;
	noise::module::Select Landscape_Select;
	noise::module::Cache Landscape_Cache;

	noise::module::RidgedMulti RiverPositions_RidgedMulti;
	noise::module::Curve RiverPositions_Curve;
	noise::module::RidgedMulti RiverPositions_RidgedMulti2;
	noise::module::Curve RiverPositions_Curve2;
	noise::module::Min RiverPositions_Min;
	noise::module::Turbulence RiverPositions_Turbulence;
	noise::module::Cache RiverPositions;
};
