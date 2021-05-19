// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "TerrainAlgorithms/HITerrainAlgorithm.h"
#include "TerrainMaths/Modules/HITerrainVoronoi.h"
#include "UObject/Object.h"
#include "VegetationAlgorithm.generated.h"

/**
 * 
 */
UCLASS()
class HITERRAINMAKER_API UVegetationAlgorithm : public UHITerrainAlgorithm
{
	GENERATED_BODY()

public:
	virtual void Init(FTerrainInformationPtr InInformation) override;
	virtual void ApplyAlgorithm(UHITerrainData* Data) override;
	virtual void DebugAlgorithm(UHITerrainData* Data) override;

	void GenerateChunkGrassData(UHITerrainData* Data, TPair<int32, int32>& Index);

private:
	FHITerrainVoronoi Voronoi;
};
