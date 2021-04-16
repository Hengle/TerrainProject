/*
* This is an independent project of an individual developer. Dear PVS-Studio, please check it.
* PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
*/
// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainAlgorithms/SampleAlgorithms/ContinentDefinitionAlgorithm.h"
#include "TerrainDatas/HITerrainData.h"

void UContinentDefinitionAlgorithm::Init(FTerrainInformationPtr InInformation)
{
	Super::Init(InInformation);
	baseContinentDef_pe0.SetSeed (Information->Seed + 0);
	baseContinentDef_pe0.SetFrequency (Information->SG_ContinentFrequency);
	baseContinentDef_pe0.SetPersistence (0.5);
	baseContinentDef_pe0.SetLacunarity (Information->SG_ContinentLacunarity);
	baseContinentDef_pe0.SetOctaveCount (14);
	baseContinentDef_pe0.SetNoiseQuality (noise::QUALITY_STD);

	baseContinentDef_cu.SetSourceModule (0, baseContinentDef_pe0);
	baseContinentDef_cu.AddControlPoint (-2.0000 + Information->SG_SeaLevel,-1.625 + Information->SG_SeaLevel);
	baseContinentDef_cu.AddControlPoint (-1.0000 + Information->SG_SeaLevel,-1.375 + Information->SG_SeaLevel);
	baseContinentDef_cu.AddControlPoint ( 0.0000 + Information->SG_SeaLevel,-0.375 + Information->SG_SeaLevel);
	baseContinentDef_cu.AddControlPoint ( 0.0625 + Information->SG_SeaLevel, 0.125 + Information->SG_SeaLevel);
	baseContinentDef_cu.AddControlPoint ( 0.1250 + Information->SG_SeaLevel, 0.250 + Information->SG_SeaLevel);
	baseContinentDef_cu.AddControlPoint ( 0.2500 + Information->SG_SeaLevel, 1.000 + Information->SG_SeaLevel);
	baseContinentDef_cu.AddControlPoint ( 0.5000 + Information->SG_SeaLevel, 0.250 + Information->SG_SeaLevel);
	baseContinentDef_cu.AddControlPoint ( 0.7500 + Information->SG_SeaLevel, 0.250 + Information->SG_SeaLevel);
	baseContinentDef_cu.AddControlPoint ( 1.0000 + Information->SG_SeaLevel, 0.500 + Information->SG_SeaLevel);
	baseContinentDef_cu.AddControlPoint ( 2.0000 + Information->SG_SeaLevel, 0.500 + Information->SG_SeaLevel);

	baseContinentDef_pe1.SetSeed (Information->Seed + 1);
	baseContinentDef_pe1.SetFrequency (Information->SG_ContinentFrequency * 4.34375);
	baseContinentDef_pe1.SetPersistence (0.5);
	baseContinentDef_pe1.SetLacunarity (Information->SG_ContinentLacunarity);
	baseContinentDef_pe1.SetOctaveCount (11);
	baseContinentDef_pe1.SetNoiseQuality (noise::QUALITY_STD);

	baseContinentDef_sb.SetSourceModule (0, baseContinentDef_pe1);
	baseContinentDef_sb.SetScale (0.375);
	baseContinentDef_sb.SetBias (0.625);

	baseContinentDef_mi.SetSourceModule (0, baseContinentDef_sb);
	baseContinentDef_mi.SetSourceModule (1, baseContinentDef_cu);

	baseContinentDef_cl.SetSourceModule (0, baseContinentDef_mi);
	baseContinentDef_cl.SetBounds (-1.0, 1.0);

	baseContinentDef.SetSourceModule (0, baseContinentDef_cl);

    continentDef_tu0.SetSourceModule (0, baseContinentDef);
    continentDef_tu0.SetSeed (Information->Seed + 10);
    continentDef_tu0.SetFrequency (Information->SG_ContinentFrequency * 15.25);
    continentDef_tu0.SetPower (Information->SG_ContinentFrequency / 113.75);
    continentDef_tu0.SetRoughness (13);

    
    continentDef_tu1.SetSourceModule (0, continentDef_tu0);
    continentDef_tu1.SetSeed (Information->Seed + 11);
    continentDef_tu1.SetFrequency (Information->SG_ContinentFrequency * 47.25);
    continentDef_tu1.SetPower (Information->SG_ContinentFrequency / 433.75);
    continentDef_tu1.SetRoughness (12);

    continentDef_tu2.SetSourceModule (0, continentDef_tu1);
    continentDef_tu2.SetSeed (Information->Seed + 12);
    continentDef_tu2.SetFrequency (Information->SG_ContinentFrequency * 95.25);
    continentDef_tu2.SetPower (Information->SG_ContinentFrequency / 1019.75);
    continentDef_tu2.SetRoughness (11);
    
    continentDef_se.SetSourceModule (0, baseContinentDef);
    continentDef_se.SetSourceModule (1, continentDef_tu2);
    continentDef_se.SetControlModule (baseContinentDef);
    continentDef_se.SetBounds (Information->SG_SeaLevel - 0.0375, Information->SG_SeaLevel + 1000.0375);
    continentDef_se.SetEdgeFalloff (0.0625);
    
    continentDef.SetSourceModule (0, continentDef_se);
}

void UContinentDefinitionAlgorithm::ApplyAlgorithm(UHITerrainData* Data)
{	
	int32 Size = Data->Size();
	for(int32 i = 0; i < Size; i++)
	{
		for(int32 j = 0; j < Size; j++)
		{
			float Value = continentDef.GetValue(i * 0.001f, j * 0.001f, 0);
			Data->SetHeightValue(i, j, Value);
		}
	}
}

const noise::module::Cache& UContinentDefinitionAlgorithm::GetContinentDef()
{
	return continentDef;
}
