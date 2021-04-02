/*
* This is an independent project of an individual developer. Dear PVS-Studio, please check it.
* PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
*/
// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainAlgorithms/SampleAlgorithms/BadLandsTerrainAlgorithm.h"

void UBadLandsTerrainAlgorithm::Init(FTerrainInformationPtr InInformation)
{
	Super::Init(InInformation);
	
    badlandsSand_rm.SetSeed (Information->Seed + 80);
    badlandsSand_rm.SetFrequency (6163.5);
    badlandsSand_rm.SetLacunarity (Information->SG_BadLandsLacunarity);
    badlandsSand_rm.SetNoiseQuality (noise::QUALITY_BEST);
    badlandsSand_rm.SetOctaveCount (1);

    
    badlandsSand_sb0.SetSourceModule (0, badlandsSand_rm);
    badlandsSand_sb0.SetScale (0.875);
    badlandsSand_sb0.SetBias (0.0);

    
    badlandsSand_vo.SetSeed (Information->Seed + 81);
    badlandsSand_vo.SetFrequency (16183.25);
    badlandsSand_vo.SetDisplacement (0.0);
    badlandsSand_vo.EnableDistance ();

    
    badlandsSand_sb1.SetSourceModule (0, badlandsSand_vo);
    badlandsSand_sb1.SetScale (0.25);
    badlandsSand_sb1.SetBias (0.25);

    
    badlandsSand_ad.SetSourceModule (0, badlandsSand_sb0);
    badlandsSand_ad.SetSourceModule (1, badlandsSand_sb1);

   
    badlandsSand.SetSourceModule (0, badlandsSand_ad);

    
    badlandsCliffs_pe.SetSeed (Information->Seed + 90);
    badlandsCliffs_pe.SetFrequency (Information->SG_ContinentFrequency * 839.0);
    badlandsCliffs_pe.SetPersistence (0.5);
    badlandsCliffs_pe.SetLacunarity (Information->SG_BadLandsLacunarity);
    badlandsCliffs_pe.SetOctaveCount (6);
    badlandsCliffs_pe.SetNoiseQuality (noise::QUALITY_STD);

    
    badlandsCliffs_cu.SetSourceModule (0, badlandsCliffs_pe);
    badlandsCliffs_cu.AddControlPoint (-2.0000, -2.0000);
    badlandsCliffs_cu.AddControlPoint (-1.0000, -1.2500);
    badlandsCliffs_cu.AddControlPoint (-0.0000, -0.7500);
    badlandsCliffs_cu.AddControlPoint ( 0.5000, -0.2500);
    badlandsCliffs_cu.AddControlPoint ( 0.6250,  0.8750);
    badlandsCliffs_cu.AddControlPoint ( 0.7500,  1.0000);
    badlandsCliffs_cu.AddControlPoint ( 2.0000,  1.2500);

    
    badlandsCliffs_cl.SetSourceModule (0, badlandsCliffs_cu);
    badlandsCliffs_cl.SetBounds (-999.125, 0.875);

    
    badlandsCliffs_te.SetSourceModule (0, badlandsCliffs_cl);
    badlandsCliffs_te.AddControlPoint (-1.0000);
    badlandsCliffs_te.AddControlPoint (-0.8750);
    badlandsCliffs_te.AddControlPoint (-0.7500);
    badlandsCliffs_te.AddControlPoint (-0.5000);
    badlandsCliffs_te.AddControlPoint ( 0.0000);
    badlandsCliffs_te.AddControlPoint ( 1.0000);

    
    badlandsCliffs_tu0.SetSeed (Information->Seed + 91);
    badlandsCliffs_tu0.SetSourceModule (0, badlandsCliffs_te);
    badlandsCliffs_tu0.SetFrequency (16111.0);
    badlandsCliffs_tu0.SetPower (1.0 / 141539.0 * Information->SG_BadLandsTwist);
    badlandsCliffs_tu0.SetRoughness (3);

    
    badlandsCliffs_tu1.SetSeed (Information->Seed + 92);
    badlandsCliffs_tu1.SetSourceModule (0, badlandsCliffs_tu0);
    badlandsCliffs_tu1.SetFrequency (36107.0);
    badlandsCliffs_tu1.SetPower (1.0 / 211543.0 * Information->SG_BadLandsTwist);
    badlandsCliffs_tu1.SetRoughness (3);

    
    badlandsCliffs.SetSourceModule (0, badlandsCliffs_tu1);

    
    badlandsTerrain_sb.SetSourceModule (0, badlandsSand);
    badlandsTerrain_sb.SetScale (0.25);
    badlandsTerrain_sb.SetBias (-0.75);

    
    badlandsTerrain_ma.SetSourceModule (0, badlandsCliffs);
    badlandsTerrain_ma.SetSourceModule (1, badlandsTerrain_sb);

    
    badlandsTerrain.SetSourceModule (0, badlandsTerrain_ma);
}

void UBadLandsTerrainAlgorithm::ApplyAlgorithm(UHITerrainData* Data)
{
	Super::ApplyAlgorithm(Data);
}

const noise::module::Cache& UBadLandsTerrainAlgorithm::GetBadLandsTerrain()
{
	return badlandsTerrain;
}
