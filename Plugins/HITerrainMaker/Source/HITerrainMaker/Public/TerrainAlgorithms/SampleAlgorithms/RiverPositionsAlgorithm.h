// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "TerrainAlgorithms/HITerrainAlgorithm.h"
#include "TerrainMaths/noiselib/noise.h"
#include "RiverPositionsAlgorithm.generated.h"

/**
 * 
 */
UCLASS()
class HITERRAINMAKER_API URiverPositionsAlgorithm : public UHITerrainAlgorithm
{
	GENERATED_BODY()

public:
	virtual void Init(FTerrainInformationPtr InInformation) override;
	virtual void ApplyAlgorithm(UHITerrainData* Data) override;

public:
	const noise::module::Cache& GetRiverPositions();

private:
	noise::module::RidgedMulti riverPositions_rm0;
	noise::module::Curve riverPositions_cu0;
	noise::module::RidgedMulti riverPositions_rm1;
	noise::module::Curve riverPositions_cu1;
	noise::module::Min riverPositions_mi;
	noise::module::Turbulence riverPositions_tu;
	noise::module::Cache riverPositions;
};


