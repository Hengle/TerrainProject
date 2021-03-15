// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainAlgorithms/MountainousTerrainAlgorithm.h"

void UMountainousTerrainAlgorithm::Init(FTerrainInformationPtr InInformation)
{
	Super::Init(InInformation);
	   
    mountainBaseDef_rm0.SetSeed (Information->Seed + 30);
    mountainBaseDef_rm0.SetFrequency (1723.0);
    mountainBaseDef_rm0.SetLacunarity (Information->MountainLacunarity);
    mountainBaseDef_rm0.SetOctaveCount (4);
    mountainBaseDef_rm0.SetNoiseQuality (noise::QUALITY_STD);
    
    mountainBaseDef_sb0.SetSourceModule (0, mountainBaseDef_rm0);
    mountainBaseDef_sb0.SetScale (0.5);
    mountainBaseDef_sb0.SetBias (0.375);
	   
    mountainBaseDef_rm1.SetSeed (Information->Seed + 31);
    mountainBaseDef_rm1.SetFrequency (367.0);
    mountainBaseDef_rm1.SetLacunarity (Information->MountainLacunarity);
    mountainBaseDef_rm1.SetOctaveCount (1);
    mountainBaseDef_rm1.SetNoiseQuality (noise::QUALITY_BEST);
    
    mountainBaseDef_sb1.SetSourceModule (0, mountainBaseDef_rm1);
    mountainBaseDef_sb1.SetScale (-2.0);
    mountainBaseDef_sb1.SetBias (-0.5);

    mountainBaseDef_co.SetConstValue (-1.0);

    mountainBaseDef_bl.SetSourceModule (0, mountainBaseDef_co);
    mountainBaseDef_bl.SetSourceModule (1, mountainBaseDef_sb0);
    mountainBaseDef_bl.SetControlModule (mountainBaseDef_sb1);
 
    mountainBaseDef_tu0.SetSourceModule (0, mountainBaseDef_bl);
    mountainBaseDef_tu0.SetSeed (Information->Seed + 32);
    mountainBaseDef_tu0.SetFrequency (1337.0);
    mountainBaseDef_tu0.SetPower (1.0 / 6730.0 * Information->MountainsTwist);
    mountainBaseDef_tu0.SetRoughness (4);

    mountainBaseDef_tu1.SetSourceModule (0, mountainBaseDef_tu0);
    mountainBaseDef_tu1.SetSeed (Information->Seed + 33);
    mountainBaseDef_tu1.SetFrequency (21221.0);
    mountainBaseDef_tu1.SetPower (1.0 / 120157.0 * Information->MountainsTwist);
    mountainBaseDef_tu1.SetRoughness (6);
 
    mountainBaseDef.SetSourceModule (0, mountainBaseDef_tu1);
 
    mountainousHigh_rm0.SetSeed (Information->Seed + 40);
    mountainousHigh_rm0.SetFrequency (2371.0);
    mountainousHigh_rm0.SetLacunarity (Information->MountainLacunarity);
    mountainousHigh_rm0.SetOctaveCount (3);
    mountainousHigh_rm0.SetNoiseQuality (noise::QUALITY_BEST);

    mountainousHigh_rm1.SetSeed (Information->Seed + 41);
    mountainousHigh_rm1.SetFrequency (2341.0);
    mountainousHigh_rm1.SetLacunarity (Information->MountainLacunarity);
    mountainousHigh_rm1.SetOctaveCount (3);
    mountainousHigh_rm1.SetNoiseQuality (noise::QUALITY_BEST);

    mountainousHigh_ma.SetSourceModule (0, mountainousHigh_rm0);
    mountainousHigh_ma.SetSourceModule (1, mountainousHigh_rm1);

    mountainousHigh_tu.SetSourceModule (0, mountainousHigh_ma);
    mountainousHigh_tu.SetSeed (Information->Seed + 42);
    mountainousHigh_tu.SetFrequency (31511.0);
    mountainousHigh_tu.SetPower (1.0 / 180371.0 * Information->MountainsTwist);
    mountainousHigh_tu.SetRoughness (4);

    mountainousHigh.SetSourceModule (0, mountainousHigh_tu);

    mountainousLow_rm0.SetSeed (Information->Seed + 50);
    mountainousLow_rm0.SetFrequency (1381.0);
    mountainousLow_rm0.SetLacunarity (Information->MountainLacunarity);
    mountainousLow_rm0.SetOctaveCount (8);
    mountainousLow_rm0.SetNoiseQuality (noise::QUALITY_BEST);

    mountainousLow_rm1.SetSeed (Information->Seed + 51);
    mountainousLow_rm1.SetFrequency (1427.0);
    mountainousLow_rm1.SetLacunarity (Information->MountainLacunarity);
    mountainousLow_rm1.SetOctaveCount (8);
    mountainousLow_rm1.SetNoiseQuality (noise::QUALITY_BEST);

    mountainousLow_mu.SetSourceModule (0, mountainousLow_rm0);
    mountainousLow_mu.SetSourceModule (1, mountainousLow_rm1);

    mountainousLow.SetSourceModule (0, mountainousLow_mu);

    mountainousTerrain_sb0.SetSourceModule (0, mountainousLow);
    mountainousTerrain_sb0.SetScale (0.03125);
    mountainousTerrain_sb0.SetBias (-0.96875);

    mountainousTerrain_sb1.SetSourceModule (0, mountainousHigh);
    mountainousTerrain_sb1.SetScale (0.25);
    mountainousTerrain_sb1.SetBias (0.25);

    mountainousTerrain_ad.SetSourceModule (0, mountainousTerrain_sb1);
    mountainousTerrain_ad.SetSourceModule (1, mountainBaseDef);

    mountainousTerrain_se.SetSourceModule (0, mountainousTerrain_sb0);
    mountainousTerrain_se.SetSourceModule (1, mountainousTerrain_ad);
    mountainousTerrain_se.SetControlModule (mountainBaseDef);
    mountainousTerrain_se.SetBounds (-0.5, 999.5);
    mountainousTerrain_se.SetEdgeFalloff (0.5);

    mountainousTerrain_sb2.SetSourceModule (0, mountainousTerrain_se);
    mountainousTerrain_sb2.SetScale (0.8);
    mountainousTerrain_sb2.SetBias (0.0);

    mountainousTerrain_ex.SetSourceModule (0, mountainousTerrain_sb2);
    mountainousTerrain_ex.SetExponent (Information->MountainGlaciation);
 
    mountainousTerrain.SetSourceModule (0, mountainousTerrain_ex);
	   
}

void UMountainousTerrainAlgorithm::Apply(UHITerrainData* Data)
{
	Super::Apply(Data);
}

const noise::module::Cache& UMountainousTerrainAlgorithm::GetMountainousTerrain()
{
	return mountainousTerrain;
}
