// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "TerrainAlgorithms/HITerrainAlgorithm.h"
#include "TerrainMaths/noiselib/noise.h"
#include "ScaledBadlandsTerrainAlgorithm.generated.h"

/**
 * 
 */
UCLASS()
class HITERRAINMAKER_API UScaledBadlandsTerrainAlgorithm : public UHITerrainAlgorithm
{
	GENERATED_BODY()

public:
	virtual void Init(FTerrainInformationPtr InInformation) override;
	virtual void ApplyAlgorithm(UHITerrainData* Data) override;

public:
	const noise::module::Cache& GetScaledBadlandsTerrain();

private:
	noise::module::ScaleBias scaledBadlandsTerrain_sb;
	noise::module::Cache scaledBadlandsTerrain;
};


