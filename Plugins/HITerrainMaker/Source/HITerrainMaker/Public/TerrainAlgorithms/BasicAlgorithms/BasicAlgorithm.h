// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "TerrainAlgorithms/HITerrainAlgorithm.h"
#include "TerrainMaths/HITerrainCurvedPerlin.h"

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
	virtual void Apply(UHITerrainData* Data) override;
	virtual void DebugApply(UHITerrainData* Data) override;

private:
	FHITerrainCurvedPerlin Landscape;
};
