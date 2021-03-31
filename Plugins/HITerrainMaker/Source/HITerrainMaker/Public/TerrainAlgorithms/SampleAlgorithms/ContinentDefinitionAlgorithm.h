// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "TerrainAlgorithms/HITerrainAlgorithm.h"
#include "TerrainMaths/noiselib/noise.h"


#include "ContinentDefinitionAlgorithm.generated.h"

/**
 * 
 */
UCLASS()
class HITERRAINMAKER_API UContinentDefinitionAlgorithm : public UHITerrainAlgorithm
{
	GENERATED_BODY()

public:
	virtual void Init(FTerrainInformationPtr InInformation) override;
	virtual void ApplyAlgorithm(UHITerrainData* Data) override;

public:
	const noise::module::Cache& GetContinentDef();

private:
	noise::module::Perlin baseContinentDef_pe0;
	noise::module::Curve baseContinentDef_cu;
	noise::module::Perlin baseContinentDef_pe1;
	noise::module::ScaleBias baseContinentDef_sb;
	noise::module::Min baseContinentDef_mi;
	noise::module::Clamp baseContinentDef_cl;
	noise::module::Cache baseContinentDef;

	noise::module::Turbulence continentDef_tu0;
	noise::module::Turbulence continentDef_tu1;
	noise::module::Turbulence continentDef_tu2;
	noise::module::Select continentDef_se;
	noise::module::Cache continentDef;
};
