// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainAlgorithms/SampleAlgorithms/TerrainTypeDefinitionAlgorithm.h"

#include "TerrainAlgorithms/SampleAlgorithms/ContinentDefinitionAlgorithm.h"

void UTerrainTypeDefinitionAlgorithm::Init(FTerrainInformationPtr InInformation)
{
	ContinentDefinitionAlgorithm = NewObject<UContinentDefinitionAlgorithm>(this);
	SubAlgorithms.Add(ContinentDefinitionAlgorithm);
	Super::Init(InInformation);
	
	terrainTypeDef_tu.SetSourceModule (0, ContinentDefinitionAlgorithm->GetContinentDef());
	terrainTypeDef_tu.SetSeed (Information->Seed + 20);
	terrainTypeDef_tu.SetFrequency (Information->SG_ContinentFrequency * 18.125);
	terrainTypeDef_tu.SetPower (Information->SG_ContinentFrequency / 20.59375 * Information->SG_TerrainOffset);
	terrainTypeDef_tu.SetRoughness (3);
	
	terrainTypeDef_te.SetSourceModule (0, terrainTypeDef_tu);
	terrainTypeDef_te.AddControlPoint (-1.00);
	terrainTypeDef_te.AddControlPoint (Information->SG_ShelfLevel + Information->SG_SeaLevel / 2.0);
	terrainTypeDef_te.AddControlPoint (1.00);

	terrainTypeDef.SetSourceModule (0, terrainTypeDef_te);
}

void UTerrainTypeDefinitionAlgorithm::ApplyAlgorithm(UHITerrainData* Data)
{
	Super::ApplyAlgorithm(Data);
}

const noise::module::Cache& UTerrainTypeDefinitionAlgorithm::GetTerrainTypeDef()
{
	return terrainTypeDef;
}

const noise::module::Cache& UTerrainTypeDefinitionAlgorithm::GetContinentDef()
{
	return ContinentDefinitionAlgorithm->GetContinentDef();
}
