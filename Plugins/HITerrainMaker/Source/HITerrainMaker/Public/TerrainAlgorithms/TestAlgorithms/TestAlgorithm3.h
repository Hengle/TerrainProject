﻿#pragma once

#include "CoreMinimal.h"

#include "TerrainAlgorithms/HITerrainAlgorithm.h"
#include "TerrainMaths/Modules/HITerrainPerlin.h"
#include "TerrainMaths/Modules/HITerrainSlopeGPU.h"
#include "UObject/Object.h"
#include "TestAlgorithm3.generated.h"

UCLASS()
class HITERRAINMAKER_API UTestAlgorithm3: public UHITerrainAlgorithm
{
	GENERATED_BODY()
public:
	virtual void Init(FTerrainInformationPtr InInformation) override;
	virtual void ApplyAlgorithm(UHITerrainData* Data) override;
	virtual void DebugAlgorithm(UHITerrainData* Data) override;

private:
	FHITerrainPerlin Perlin;
	FHITerrainSlopeGPU SlopeGPU;
};
