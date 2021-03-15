// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "HITerrainAlgorithm.h"
#include "TerrainMaths/noiselib/noise.h"
#include "HillyTerrainAlgorithm.generated.h"

/**
 * 
 */
UCLASS()
class HITERRAINMAKER_API UHillyTerrainAlgorithm : public UHITerrainAlgorithm
{
	GENERATED_BODY()

public:
	virtual void Init(FTerrainInformationPtr InInformation) override;
	virtual void Apply(UHITerrainData* Data) override;

public:
	const noise::module::Cache& GetHillyTerrain();

private:
	noise::module::Billow hillyTerrain_bi;
	noise::module::ScaleBias hillyTerrain_sb0;
	noise::module::RidgedMulti hillyTerrain_rm;
	noise::module::ScaleBias hillyTerrain_sb1;
	noise::module::Const hillyTerrain_co;
	noise::module::Blend hillyTerrain_bl;
	noise::module::ScaleBias hillyTerrain_sb2;
	noise::module::Exponent hillyTerrain_ex;
	noise::module::Turbulence hillyTerrain_tu0;
	noise::module::Turbulence hillyTerrain_tu1;
	noise::module::Cache hillyTerrain;
};
