// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "HITerrainAlgorithm.h"
#include "TerrainMaths/noiselib/noise.h"
#include "PlainsTerrainAlgorithm.generated.h"

/**
 * 
 */
UCLASS()
class HITERRAINMAKER_API UPlainsTerrainAlgorithm : public UHITerrainAlgorithm
{
	GENERATED_BODY()

public:
	virtual void Init(FTerrainInformationPtr InInformation) override;
	virtual void Apply(UHITerrainData* Data) override;

public:
	const noise::module::Cache& GetPlainsTerrain();

private:
	noise::module::Billow plainsTerrain_bi0;
	noise::module::ScaleBias plainsTerrain_sb0;
	noise::module::Billow plainsTerrain_bi1;
	noise::module::ScaleBias plainsTerrain_sb1;
	noise::module::Multiply plainsTerrain_mu;
	noise::module::ScaleBias plainsTerrain_sb2;
	noise::module::Cache plainsTerrain;
};
