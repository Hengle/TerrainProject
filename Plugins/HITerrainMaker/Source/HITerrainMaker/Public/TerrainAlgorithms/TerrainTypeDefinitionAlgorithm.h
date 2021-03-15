// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"


#include "ContinentDefinitionAlgorithm.h"
#include "HITerrainAlgorithm.h"
#include "TerrainMaths/noiselib/noise.h"
#include "TerrainTypeDefinitionAlgorithm.generated.h"

/**
 * 
 */
UCLASS()
class HITERRAINMAKER_API UTerrainTypeDefinitionAlgorithm : public UHITerrainAlgorithm
{
	GENERATED_BODY()

public:
	virtual void Init(FTerrainInformationPtr InInformation) override;
	virtual void Apply(UHITerrainData* Data) override;

public:
	const noise::module::Cache& GetTerrainTypeDef();
	const noise::module::Cache& GetContinentDef();

private:
	UContinentDefinitionAlgorithm* ContinentDefinitionAlgorithm;
	
private:
	noise::module::Turbulence terrainTypeDef_tu;
	noise::module::Terrace terrainTypeDef_te;
	noise::module::Cache terrainTypeDef;
};
