// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "HITerrainAlgorithm.h"
#include "TerrainMaths/noiselib/noise.h"
#include "ScaledHillyTerrainAlgorithm.generated.h"

/**
 * 
 */
UCLASS()
class HITERRAINMAKER_API UScaledHillyTerrainAlgorithm : public UHITerrainAlgorithm
{
	GENERATED_BODY()

public:
	virtual void Init(FTerrainInformationPtr InInformation) override;
	virtual void Apply(UHITerrainData* Data) override;

public:
	const noise::module::Cache& GetScaledHillyTerrain();

private:
	noise::module::ScaleBias scaledHillyTerrain_sb0;
	noise::module::Perlin scaledHillyTerrain_pe;
	noise::module::Exponent scaledHillyTerrain_ex;
	noise::module::ScaleBias scaledHillyTerrain_sb1;
	noise::module::Multiply scaledHillyTerrain_mu;
	noise::module::Cache scaledHillyTerrain;
};

