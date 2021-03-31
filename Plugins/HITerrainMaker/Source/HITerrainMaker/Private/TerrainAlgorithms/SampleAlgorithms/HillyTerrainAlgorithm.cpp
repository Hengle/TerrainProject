// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainAlgorithms/SampleAlgorithms/HillyTerrainAlgorithm.h"

void UHillyTerrainAlgorithm::Init(FTerrainInformationPtr InInformation)
{
	Super::Init(InInformation);
	
    hillyTerrain_bi.SetSeed (Information->Seed + 60);
    hillyTerrain_bi.SetFrequency (1663.0);
    hillyTerrain_bi.SetPersistence (0.5);
    hillyTerrain_bi.SetLacunarity (Information->SG_HillsLacunarity);
    hillyTerrain_bi.SetOctaveCount (6);
    hillyTerrain_bi.SetNoiseQuality (noise::QUALITY_BEST);

    
    hillyTerrain_sb0.SetSourceModule (0, hillyTerrain_bi);
    hillyTerrain_sb0.SetScale (0.5);
    hillyTerrain_sb0.SetBias (0.5);
	
    
    hillyTerrain_rm.SetSeed (Information->Seed + 61);
    hillyTerrain_rm.SetFrequency (367.5);
    hillyTerrain_rm.SetLacunarity (Information->SG_HillsLacunarity);
    hillyTerrain_rm.SetNoiseQuality (noise::QUALITY_BEST);
    hillyTerrain_rm.SetOctaveCount (1);
	
    
    hillyTerrain_sb1.SetSourceModule (0, hillyTerrain_rm);
    hillyTerrain_sb1.SetScale (-2.0);
    hillyTerrain_sb1.SetBias (-0.5);

   
    hillyTerrain_co.SetConstValue (-1.0);


    hillyTerrain_bl.SetSourceModule (0, hillyTerrain_co);
    hillyTerrain_bl.SetSourceModule (1, hillyTerrain_sb1);
    hillyTerrain_bl.SetControlModule (hillyTerrain_sb0);

   
    hillyTerrain_sb2.SetSourceModule (0, hillyTerrain_bl);
    hillyTerrain_sb2.SetScale (0.75);
    hillyTerrain_sb2.SetBias (-0.25);


    hillyTerrain_ex.SetSourceModule (0, hillyTerrain_sb2);
    hillyTerrain_ex.SetExponent (1.375);


    hillyTerrain_tu0.SetSourceModule (0, hillyTerrain_ex);
    hillyTerrain_tu0.SetSeed (Information->Seed + 62);
    hillyTerrain_tu0.SetFrequency (1531.0);
    hillyTerrain_tu0.SetPower (1.0 / 16921.0 * Information->SG_HillsTwist);
    hillyTerrain_tu0.SetRoughness (4);


    hillyTerrain_tu1.SetSourceModule (0, hillyTerrain_tu0);
    hillyTerrain_tu1.SetSeed (Information->Seed + 63);
    hillyTerrain_tu1.SetFrequency (21617.0);
    hillyTerrain_tu1.SetPower (1.0 / 117529.0 * Information->SG_HillsTwist);
    hillyTerrain_tu1.SetRoughness (6);


    hillyTerrain.SetSourceModule (0, hillyTerrain_tu1);
}

void UHillyTerrainAlgorithm::ApplyAlgorithm(UHITerrainData* Data)
{
	Super::ApplyAlgorithm(Data);
}

const noise::module::Cache& UHillyTerrainAlgorithm::GetHillyTerrain()
{
	return hillyTerrain;
}
