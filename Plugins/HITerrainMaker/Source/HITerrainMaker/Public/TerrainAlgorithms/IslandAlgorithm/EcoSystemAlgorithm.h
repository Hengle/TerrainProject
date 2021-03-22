// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "TerrainAlgorithms/HITerrainAlgorithm.h"
#include "TerrainMaths/HITerrainVoronoi.h"
#include "TerrainMaths/noiselib/noise.h"
#include "EcoSystemAlgorithm.generated.h"

/**
 * 
 */
UCLASS()
class HITERRAINMAKER_API UEcoSystemAlgorithm : public UHITerrainAlgorithm
{
	GENERATED_BODY()

public:
	virtual void Init(FTerrainInformationPtr InInformation) override;
	virtual void Apply(UHITerrainData* Data) override;
	virtual void DebugApply(UHITerrainData* Data) override;

private:
	FHITerrainVoronoi Voronoi;
	noise::module::Perlin Beach;
	noise::module::Perlin Perlin;
	noise::module::Perlin SmallIsland;
	// noise::module::Voronoi Voronoi;
};
