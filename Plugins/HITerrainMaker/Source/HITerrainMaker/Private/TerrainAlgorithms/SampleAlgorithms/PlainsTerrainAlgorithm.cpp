// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainAlgorithms/SampleAlgorithms/PlainsTerrainAlgorithm.h"

void UPlainsTerrainAlgorithm::Init(FTerrainInformationPtr InInformation)
{
	Super::Init(InInformation);
	
	
    plainsTerrain_bi0.SetSeed (Information->Seed + 70);
    plainsTerrain_bi0.SetFrequency (1097.5);
    plainsTerrain_bi0.SetPersistence (0.5);
    plainsTerrain_bi0.SetLacunarity (Information->SG_PlainsLacunarity);
    plainsTerrain_bi0.SetOctaveCount (8);
    plainsTerrain_bi0.SetNoiseQuality (noise::QUALITY_BEST);

    
    plainsTerrain_sb0.SetSourceModule (0, plainsTerrain_bi0);
    plainsTerrain_sb0.SetScale (0.5);
    plainsTerrain_sb0.SetBias (0.5);

    
    plainsTerrain_bi1.SetSeed (Information->Seed + 71);
    plainsTerrain_bi1.SetFrequency (1319.5);
    plainsTerrain_bi1.SetPersistence (0.5);
    plainsTerrain_bi1.SetLacunarity (Information->SG_PlainsLacunarity);
    plainsTerrain_bi1.SetOctaveCount (8);
    plainsTerrain_bi1.SetNoiseQuality (noise::QUALITY_BEST);

    
    plainsTerrain_sb1.SetSourceModule (0, plainsTerrain_bi1);
    plainsTerrain_sb1.SetScale (0.5);
    plainsTerrain_sb1.SetBias (0.5);

    
    plainsTerrain_mu.SetSourceModule (0, plainsTerrain_sb0);
    plainsTerrain_mu.SetSourceModule (1, plainsTerrain_sb1);

    
    plainsTerrain_sb2.SetSourceModule (0, plainsTerrain_mu);
    plainsTerrain_sb2.SetScale (2.0);
    plainsTerrain_sb2.SetBias (-1.0);

    
    plainsTerrain.SetSourceModule (0, plainsTerrain_sb2);
}

void UPlainsTerrainAlgorithm::Apply(UHITerrainData* Data)
{
	Super::Apply(Data);
}

const noise::module::Cache& UPlainsTerrainAlgorithm::GetPlainsTerrain()
{
	return plainsTerrain;
}
