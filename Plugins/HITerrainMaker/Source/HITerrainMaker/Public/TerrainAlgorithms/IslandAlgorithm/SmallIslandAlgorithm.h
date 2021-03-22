// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "TerrainAlgorithms/HITerrainAlgorithm.h"
#include "TerrainMaths/HITerrainCurvedPerlin.h"
#include "TerrainMaths/HITerrainVoronoi.h"
#include "TerrainMaths/noiselib/noise.h"
#include "SmallIslandAlgorithm.generated.h"

/**
 * 
 */
UCLASS()
class HITERRAINMAKER_API USmallIslandAlgorithm : public UHITerrainAlgorithm
{
	GENERATED_BODY()

public:
	virtual void Init(FTerrainInformationPtr InInformation) override;
	virtual void Apply(UHITerrainData* Data) override;
	virtual void DebugApply(UHITerrainData* Data) override;

private:
	void GenerateOcean();

	void GenerateMainIsland();

private:
	FHITerrainVoronoi Voronoi;
	FHITerrainCurvedPerlin Perlin;

private:
	TArray<FVoronoiCellInfo> AllCells;
	TMap<int32, ESampleType> CellTypes;
};
