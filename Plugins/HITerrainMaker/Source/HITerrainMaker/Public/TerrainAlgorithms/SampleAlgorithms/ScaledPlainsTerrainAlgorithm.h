// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "TerrainAlgorithms/HITerrainAlgorithm.h"
#include "TerrainMaths/noiselib/noise.h"
#include "ScaledPlainsTerrainAlgorithm.generated.h"

/**
 * 
 */
UCLASS()
class HITERRAINMAKER_API UScaledPlainsTerrainAlgorithm : public UHITerrainAlgorithm
{
	GENERATED_BODY()

public:
	virtual void Init(FTerrainInformationPtr InInformation) override;
	virtual void Apply(UHITerrainData* Data) override;

public:
	const noise::module::Cache& GetScaledPlainsTerrain();
	
private:
	noise::module::ScaleBias scaledPlainsTerrain_sb;
	noise::module::Cache scaledPlainsTerrain;
};
