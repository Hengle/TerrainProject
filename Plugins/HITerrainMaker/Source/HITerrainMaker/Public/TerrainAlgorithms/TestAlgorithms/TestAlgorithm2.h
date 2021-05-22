// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "TerrainAlgorithms/HITerrainAlgorithm.h"
#include "TerrainMaths/Modules/HITerrainErosion.h"
#include "TerrainMaths/Modules/HITerrainErosionGPU.h"
#include "TerrainMaths/Modules/HITerrainPerlin.h"
#include "TerrainMaths/Modules/HITerrainWaterFlattenGPU.h"
#include "TerrainMaths/Modules/ThreadSafeTest.h"
#include "UObject/Object.h"
#include "TestAlgorithm2.generated.h"

/**
 * 
 */
UCLASS()
class HITERRAINMAKER_API UTestAlgorithm2 : public UHITerrainAlgorithm
{
	GENERATED_BODY()

	public:
	virtual void Init(FTerrainInformationPtr InInformation) override;
	virtual void ApplyAlgorithm(UHITerrainData* Data) override;
	virtual void DebugAlgorithm(UHITerrainData* Data) override;

	private:
	FHITerrainPerlin Perlin;
	FHITerrainErosionGPU ErosionGPU;
	FHITerrainWaterFlattenGPU WaterFlattenGPU;
	FThreadSafeTest ThreadSafeTest;
};
