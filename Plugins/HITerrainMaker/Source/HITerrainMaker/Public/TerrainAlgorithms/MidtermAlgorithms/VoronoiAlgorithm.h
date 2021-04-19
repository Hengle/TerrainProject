// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "TerrainAlgorithms/HITerrainAlgorithm.h"
#include "TerrainMaths/Modules/HITerrainVoronoi.h"
#include "VoronoiAlgorithm.generated.h"

/**
 * 
 */
UCLASS()
class HITERRAINMAKER_API UVoronoiAlgorithm : public UHITerrainAlgorithm
{
	GENERATED_BODY()

public:
	virtual void Init(FTerrainInformationPtr InInformation) override;
	virtual void ApplyAlgorithm(UHITerrainData* Data) override;
	virtual void DebugAlgorithm(UHITerrainData* Data) override;

private:
	FHITerrainVoronoi Voronoi;

private:
	TArray<FVoronoiCellInfo> AllCells;
	TMap<int32, ESampleType> CellTypes;
};
