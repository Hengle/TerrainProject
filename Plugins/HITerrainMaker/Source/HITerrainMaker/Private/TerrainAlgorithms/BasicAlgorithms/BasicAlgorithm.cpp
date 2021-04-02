/*
* This is an independent project of an individual developer. Dear PVS-Studio, please check it.
* PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
*/
// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainAlgorithms/BasicAlgorithms/BasicAlgorithm.h"

#include <destructible/ExplicitHierarchicalMesh.h>

#include "TerrainDatas/HITerrainData.h"

void UBasicAlgorithm::Init(FTerrainInformationPtr InInformation)
{
	Super::Init(InInformation);
	BaseLandscape.SetSeed(Information->Seed);
	BaseLandscape.SetFrequency(Information->BG_LandscapeFrequency);
	BaseLandscape.SetLacunarity(Information->BG_LandscapeLacunarity);
	BaseLandscape.SetOctaveCount(Information->BG_LandscapeOctaveCount);
	BaseLandscape.SetNoiseQuality(noise::QUALITY_STD);
	BaseLandscape.SetPersistence(0.5f);

	BaseLandscape_Curve.SetSourceModule(0, BaseLandscape);
	BaseLandscape_Curve.AddControlPoint(-2.0000 + Information->BG_SeaLevel,-1.625 + Information->BG_SeaLevel);
	BaseLandscape_Curve.AddControlPoint(-1.0000 + Information->BG_SeaLevel,-1.375 + Information->BG_SeaLevel);
	BaseLandscape_Curve.AddControlPoint( 0.0000 + Information->BG_SeaLevel,-0.375 + Information->BG_SeaLevel);
	BaseLandscape_Curve.AddControlPoint( 0.0625 + Information->BG_SeaLevel, 0.125 + Information->BG_SeaLevel);
	BaseLandscape_Curve.AddControlPoint( 0.1250 + Information->BG_SeaLevel, 0.250 + Information->BG_SeaLevel);
	BaseLandscape_Curve.AddControlPoint( 0.2500 + Information->BG_SeaLevel, 1.000 + Information->BG_SeaLevel);
	BaseLandscape_Curve.AddControlPoint( 0.5000 + Information->BG_SeaLevel, 0.250 + Information->BG_SeaLevel);
	BaseLandscape_Curve.AddControlPoint( 0.7500 + Information->BG_SeaLevel, 0.250 + Information->BG_SeaLevel);
	BaseLandscape_Curve.AddControlPoint( 1.0000 + Information->BG_SeaLevel, 0.500 + Information->BG_SeaLevel);
	BaseLandscape_Curve.AddControlPoint( 2.0000 + Information->BG_SeaLevel, 0.500 + Information->BG_SeaLevel);

	BaseLandscape2.SetSeed(Information->Seed + 1);
	BaseLandscape2.SetFrequency(Information->BG_LandscapeFrequency * 4.34375);
	BaseLandscape2.SetPersistence(0.5);
	BaseLandscape2.SetLacunarity(Information->BG_LandscapeLacunarity);
	BaseLandscape2.SetOctaveCount(Information->BG_LandscapeOctaveCount);
	BaseLandscape2.SetNoiseQuality(noise::QUALITY_STD);

	BaseLandscape2_ScaleBias.SetSourceModule (0, BaseLandscape2);
	BaseLandscape2_ScaleBias.SetScale (0.375);
	BaseLandscape2_ScaleBias.SetBias (0.625);

	BaseLandscape_Min.SetSourceModule(0, BaseLandscape_Curve);
	BaseLandscape_Min.SetSourceModule(1, BaseLandscape2_ScaleBias);

	BaseLandscape_Clamp.SetSourceModule (0, BaseLandscape_Min);
	BaseLandscape_Clamp.SetBounds (-1.0, 1.0);

	BaseLandscape_Cache.SetSourceModule(0, BaseLandscape_Clamp);


	Landscape_Turbulence1.SetSourceModule (0, BaseLandscape_Cache);
	Landscape_Turbulence1.SetSeed (Information->Seed + 10);
	Landscape_Turbulence1.SetFrequency (Information->BG_LandscapeFrequency * 2.25);
	Landscape_Turbulence1.SetPower (Information->BG_LandscapeFrequency / 1.75);
	Landscape_Turbulence1.SetRoughness (13);

	Landscape_Turbulence2.SetSourceModule (0, Landscape_Turbulence1);
	Landscape_Turbulence2.SetSeed (Information->Seed + 11);
	Landscape_Turbulence2.SetFrequency (Information->BG_LandscapeFrequency * 4.25);
	Landscape_Turbulence2.SetPower (Information->BG_LandscapeFrequency / 3.75);
	Landscape_Turbulence2.SetRoughness (12);

	Landscape_Turbulence3.SetSourceModule (0, Landscape_Turbulence2);
	Landscape_Turbulence3.SetSeed (Information->Seed + 12);
	Landscape_Turbulence3.SetFrequency (Information->BG_LandscapeFrequency * 8.25);
	Landscape_Turbulence3.SetPower (Information->BG_LandscapeFrequency / 6.75);
	Landscape_Turbulence3.SetRoughness (11);

	Landscape_Select.SetSourceModule (0, BaseLandscape_Cache);
	Landscape_Select.SetSourceModule (1, Landscape_Turbulence3);
	Landscape_Select.SetControlModule (BaseLandscape_Cache);
	Landscape_Select.SetBounds (Information->BG_SeaLevel - 0.0375, Information->BG_SeaLevel + 1000.0375);
	Landscape_Select.SetEdgeFalloff (0.0625);

	Landscape_Cache.SetSourceModule(0, Landscape_Select);




	
    RiverPositions_RidgedMulti.SetSeed (Information->Seed + 100);
    RiverPositions_RidgedMulti.SetFrequency (0.5);
    RiverPositions_RidgedMulti.SetLacunarity (Information->BG_LandscapeLacunarity);
    RiverPositions_RidgedMulti.SetOctaveCount (1);
    RiverPositions_RidgedMulti.SetNoiseQuality (noise::QUALITY_BEST);

    
    RiverPositions_Curve.SetSourceModule (0, RiverPositions_RidgedMulti);
    RiverPositions_Curve.AddControlPoint (-2.000,  2.000);
    RiverPositions_Curve.AddControlPoint (-1.000,  1.000);
    RiverPositions_Curve.AddControlPoint (-0.125,  0.875);
    RiverPositions_Curve.AddControlPoint ( 0.000, -1.000);
    RiverPositions_Curve.AddControlPoint ( 1.000, -1.500);
    RiverPositions_Curve.AddControlPoint ( 2.000, -2.000);

    
    RiverPositions_RidgedMulti2.SetSeed (Information->Seed + 101);
    RiverPositions_RidgedMulti2.SetFrequency (1);
    RiverPositions_RidgedMulti2.SetLacunarity (Information->BG_LandscapeLacunarity);
    RiverPositions_RidgedMulti2.SetOctaveCount (1);
    RiverPositions_RidgedMulti2.SetNoiseQuality (noise::QUALITY_BEST);

    
    RiverPositions_Curve2.SetSourceModule (0, RiverPositions_RidgedMulti2);
    RiverPositions_Curve2.AddControlPoint (-2.000,  2.0000);
    RiverPositions_Curve2.AddControlPoint (-1.000,  1.5000);
    RiverPositions_Curve2.AddControlPoint (-0.125,  1.4375);
    RiverPositions_Curve2.AddControlPoint ( 0.000,  0.5000);
    RiverPositions_Curve2.AddControlPoint ( 1.000,  0.2500);
    RiverPositions_Curve2.AddControlPoint ( 2.000,  0.0000);

    
    RiverPositions_Min.SetSourceModule (0, RiverPositions_Curve);
    RiverPositions_Min.SetSourceModule (1, RiverPositions_Curve2);

	// RiverPositions_Turbulence.SetSourceModule (0, RiverPositions_Min);
    RiverPositions_Turbulence.SetSourceModule (0, RiverPositions_Curve2);
    RiverPositions_Turbulence.SetSeed (Information->Seed + 102);
    RiverPositions_Turbulence.SetFrequency (9.25);
    RiverPositions_Turbulence.SetPower (1.0 / 57.75);
    RiverPositions_Turbulence.SetRoughness (6);

    
    RiverPositions.SetSourceModule (0, RiverPositions_Turbulence);

}

void UBasicAlgorithm::ApplyAlgorithm(UHITerrainData* Data)
{
	Super::ApplyAlgorithm(Data);
	int32 Size = Data->Size();
	for(int32 i = 0; i < Size; i++)
	{
		for(int32 j = 0; j < Size; j++)
		{
			float Value = 0.0f;
			Value = Landscape_Cache.GetValue(i * 0.01f, j * 0.01f, 0.0f) * 2000;
			Data->SetSampleValue(i, j, Value);
		}
	}
}

void UBasicAlgorithm::DebugAlgorithm(UHITerrainData* Data)
{
	Super::DebugAlgorithm(Data);
	int32 Size = Data->Size();
	for(int32 i = 0; i < Size; i++)
	{
		for(int32 j = 0; j < Size; j++)
		{
			float Value = 0.0f;
			Value = RiverPositions.GetValue(i * 0.01f, j * 0.01f, 0.0f) * 2000;
			Data->SetSampleValue(i, j, Value);
		}
	}
}
