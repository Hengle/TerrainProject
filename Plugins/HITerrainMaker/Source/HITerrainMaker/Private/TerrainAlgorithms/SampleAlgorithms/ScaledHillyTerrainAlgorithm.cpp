/*
* This is an independent project of an individual developer. Dear PVS-Studio, please check it.
* PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
*/
// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainAlgorithms/SampleAlgorithms/ScaledHillyTerrainAlgorithm.h"

#include "TerrainAlgorithms/SampleAlgorithms/HillyTerrainAlgorithm.h"

void UScaledHillyTerrainAlgorithm::Init(FTerrainInformationPtr InInformation)
{
	UHillyTerrainAlgorithm* HillyTerrainAlgorithm = NewObject<UHillyTerrainAlgorithm>(this);
	SubAlgorithms.Add(HillyTerrainAlgorithm);
	Super::Init(InInformation);
	
    scaledHillyTerrain_sb0.SetSourceModule (0, HillyTerrainAlgorithm->GetHillyTerrain());
    scaledHillyTerrain_sb0.SetScale (0.0625);
    scaledHillyTerrain_sb0.SetBias (0.0625);

    
    scaledHillyTerrain_pe.SetSeed (Information->Seed + 120);
    scaledHillyTerrain_pe.SetFrequency (13.5);
    scaledHillyTerrain_pe.SetPersistence (0.5);
    scaledHillyTerrain_pe.SetLacunarity (Information->SG_HillsLacunarity);
    scaledHillyTerrain_pe.SetOctaveCount (6);
    scaledHillyTerrain_pe.SetNoiseQuality (noise::QUALITY_STD);

    
    scaledHillyTerrain_ex.SetSourceModule (0, scaledHillyTerrain_pe);
    scaledHillyTerrain_ex.SetExponent (1.25);

    
    scaledHillyTerrain_sb1.SetSourceModule (0, scaledHillyTerrain_ex);
    scaledHillyTerrain_sb1.SetScale (0.5);
    scaledHillyTerrain_sb1.SetBias (1.5);

    
    scaledHillyTerrain_mu.SetSourceModule (0, scaledHillyTerrain_sb0);
    scaledHillyTerrain_mu.SetSourceModule (1, scaledHillyTerrain_sb1);

    
    scaledHillyTerrain.SetSourceModule (0, scaledHillyTerrain_mu);
}

void UScaledHillyTerrainAlgorithm::ApplyAlgorithm(UHITerrainData* Data)
{
	Super::ApplyAlgorithm(Data);
}

const noise::module::Cache& UScaledHillyTerrainAlgorithm::GetScaledHillyTerrain()
{
	return scaledHillyTerrain;
}
