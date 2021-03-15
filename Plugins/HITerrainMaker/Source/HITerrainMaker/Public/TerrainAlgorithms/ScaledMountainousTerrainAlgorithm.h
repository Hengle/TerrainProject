// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "HITerrainAlgorithm.h"
#include "TerrainMaths/noiselib/noise.h"
#include "ScaledMountainousTerrainAlgorithm.generated.h"

/**
 * 
 */
UCLASS()
class HITERRAINMAKER_API UScaledMountainousTerrainAlgorithm : public UHITerrainAlgorithm
{
	GENERATED_BODY()

public:
	virtual void Init(FTerrainInformationPtr InInformation) override;
	virtual void Apply(UHITerrainData* Data) override;

public:
	const noise::module::Cache& GetScaledMountainousTerrain();

private:
	noise::module::ScaleBias scaledMountainousTerrain_sb0;
	noise::module::Perlin scaledMountainousTerrain_pe;
	noise::module::Exponent scaledMountainousTerrain_ex;
	noise::module::ScaleBias scaledMountainousTerrain_sb1;
	noise::module::Multiply scaledMountainousTerrain_mu;
	noise::module::Cache scaledMountainousTerrain;
};


