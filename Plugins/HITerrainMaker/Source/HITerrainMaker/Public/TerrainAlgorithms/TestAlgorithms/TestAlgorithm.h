﻿#pragma once

#include "CoreMinimal.h"

#include "TerrainAlgorithms/HITerrainAlgorithm.h"
#include "TerrainMaths/Modules/HITerrainErosion.h"
#include "TerrainMaths/Modules/HITerrainPerlin.h"
#include "UObject/Object.h"
#include "TestAlgorithm.generated.h"

UCLASS()
class HITERRAINMAKER_API UTestAlgorithm : public UHITerrainAlgorithm
{
	GENERATED_BODY()

public:
	virtual void Init(FTerrainInformationPtr InInformation) override;
	virtual void ApplyAlgorithm(UHITerrainData* Data) override;
	virtual void DebugAlgorithm(UHITerrainData* Data) override;

private:
	FHITerrainPerlin Perlin;
	FHITerrainErosion Erosion;
};
