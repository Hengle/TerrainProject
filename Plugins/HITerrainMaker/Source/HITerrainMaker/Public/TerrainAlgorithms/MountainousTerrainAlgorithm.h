// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "HITerrainAlgorithm.h"
#include "TerrainMaths/noiselib/noise.h"
#include "MountainousTerrainAlgorithm.generated.h"

/**
 * 
 */
UCLASS()
class HITERRAINMAKER_API UMountainousTerrainAlgorithm : public UHITerrainAlgorithm
{
	GENERATED_BODY()

public:
	virtual void Init(FTerrainInformationPtr InInformation) override;
	virtual void Apply(UHITerrainData* Data) override;

public:
	const noise::module::Cache& GetMountainousTerrain();

private:
	noise::module::RidgedMulti mountainBaseDef_rm0;
	noise::module::ScaleBias mountainBaseDef_sb0;
	noise::module::RidgedMulti mountainBaseDef_rm1;
	noise::module::ScaleBias mountainBaseDef_sb1;
	noise::module::Const mountainBaseDef_co;
	noise::module::Blend mountainBaseDef_bl;
	noise::module::Turbulence mountainBaseDef_tu0;
	noise::module::Turbulence mountainBaseDef_tu1;
	noise::module::Cache mountainBaseDef;
	noise::module::RidgedMulti mountainousHigh_rm0;
	noise::module::RidgedMulti mountainousHigh_rm1;
	noise::module::Max mountainousHigh_ma;
	noise::module::Turbulence mountainousHigh_tu;
	noise::module::Cache mountainousHigh;
	noise::module::RidgedMulti mountainousLow_rm0;
	noise::module::RidgedMulti mountainousLow_rm1;
	noise::module::Multiply mountainousLow_mu;
	noise::module::Cache mountainousLow;
	noise::module::ScaleBias mountainousTerrain_sb0;
	noise::module::ScaleBias mountainousTerrain_sb1;
	noise::module::Add mountainousTerrain_ad;
	noise::module::Select mountainousTerrain_se;
	noise::module::ScaleBias mountainousTerrain_sb2;
	noise::module::Exponent mountainousTerrain_ex;
	noise::module::Cache mountainousTerrain;
};
