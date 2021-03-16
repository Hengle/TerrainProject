// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "HITerrainAlgorithm.h"
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
	noise::module::Voronoi Voronoi;
};
