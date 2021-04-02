/*
* This is an independent project of an individual developer. Dear PVS-Studio, please check it.
* PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
*/
// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainAlgorithms/SampleAlgorithms/RiverPositionsAlgorithm.h"

void URiverPositionsAlgorithm::Init(FTerrainInformationPtr InInformation)
{
	Super::Init(InInformation);
	
    riverPositions_rm0.SetSeed (Information->Seed + 100);
    riverPositions_rm0.SetFrequency (18.75);
    riverPositions_rm0.SetLacunarity (Information->SG_ContinentLacunarity);
    riverPositions_rm0.SetOctaveCount (1);
    riverPositions_rm0.SetNoiseQuality (noise::QUALITY_BEST);

    
    riverPositions_cu0.SetSourceModule (0, riverPositions_rm0);
    riverPositions_cu0.AddControlPoint (-2.000,  2.000);
    riverPositions_cu0.AddControlPoint (-1.000,  1.000);
    riverPositions_cu0.AddControlPoint (-0.125,  0.875);
    riverPositions_cu0.AddControlPoint ( 0.000, -1.000);
    riverPositions_cu0.AddControlPoint ( 1.000, -1.500);
    riverPositions_cu0.AddControlPoint ( 2.000, -2.000);

    
    riverPositions_rm1.SetSeed (Information->Seed + 101);
    riverPositions_rm1.SetFrequency (43.25);
    riverPositions_rm1.SetLacunarity (Information->SG_ContinentLacunarity);
    riverPositions_rm1.SetOctaveCount (1);
    riverPositions_rm1.SetNoiseQuality (noise::QUALITY_BEST);

    
    riverPositions_cu1.SetSourceModule (0, riverPositions_rm1);
    riverPositions_cu1.AddControlPoint (-2.000,  2.0000);
    riverPositions_cu1.AddControlPoint (-1.000,  1.5000);
    riverPositions_cu1.AddControlPoint (-0.125,  1.4375);
    riverPositions_cu1.AddControlPoint ( 0.000,  0.5000);
    riverPositions_cu1.AddControlPoint ( 1.000,  0.2500);
    riverPositions_cu1.AddControlPoint ( 2.000,  0.0000);

    
    riverPositions_mi.SetSourceModule (0, riverPositions_cu0);
    riverPositions_mi.SetSourceModule (1, riverPositions_cu1);

   
    riverPositions_tu.SetSourceModule (0, riverPositions_mi);
    riverPositions_tu.SetSeed (Information->Seed + 102);
    riverPositions_tu.SetFrequency (9.25);
    riverPositions_tu.SetPower (1.0 / 57.75);
    riverPositions_tu.SetRoughness (6);

    
    riverPositions.SetSourceModule (0, riverPositions_tu);
}

void URiverPositionsAlgorithm::ApplyAlgorithm(UHITerrainData* Data)
{
	Super::ApplyAlgorithm(Data);
}

const noise::module::Cache& URiverPositionsAlgorithm::GetRiverPositions()
{
	return riverPositions;
}