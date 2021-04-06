// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainAlgorithms/MidtermAlgorithms/PerlinAlgorithm.h"

#include "TerrainDatas/HITerrainData.h"

void UPerlinAlgorithm::Init(FTerrainInformationPtr InInformation)
{
	Super::Init(InInformation);
	BaseLandscape.SetSeed(Information->Seed);
	BaseLandscape.SetFrequency(Information->BG_LandscapeFrequency);
	BaseLandscape.SetLacunarity(Information->BG_LandscapeLacunarity);
	BaseLandscape.SetOctaveCount(Information->BG_LandscapeOctaveCount);
	BaseLandscape.SetNoiseQuality(noise::QUALITY_STD);
	BaseLandscape.SetPersistence(0.5f);

	BaseLandscape_Curve.SetSourceModule(0, BaseLandscape);
	BaseLandscape_Curve.AddControlPoint(-1.0f, -1.0f);
	BaseLandscape_Curve.AddControlPoint(-0.5f, -0.8f);
	BaseLandscape_Curve.AddControlPoint(-0.25f, 0.0f);
	BaseLandscape_Curve.AddControlPoint(0.6f, 0.25f);
	BaseLandscape_Curve.AddControlPoint(0.7f, 0.5f);
	BaseLandscape_Curve.AddControlPoint(0.8f, 0.75f);
	BaseLandscape_Curve.AddControlPoint(0.9f, 1.25f);
	BaseLandscape_Curve.AddControlPoint(1.0f, 2.25f);

	BaseLandscape_Terrace.SetSourceModule(0, BaseLandscape_Curve);
	BaseLandscape_Terrace.AddControlPoint(-1.0f);
	BaseLandscape_Terrace.AddControlPoint(-0.1f);
	BaseLandscape_Terrace.AddControlPoint(0.2f);
	BaseLandscape_Terrace.AddControlPoint(1.0f);
	BaseLandscape_Terrace.AddControlPoint(2.25f);
	BaseLandscape_Terrace.AddControlPoint(2.5f);

	BaseLandscape_Cache.SetSourceModule(0, BaseLandscape_Terrace);
}

void UPerlinAlgorithm::ApplyAlgorithm(UHITerrainData* Data)
{
	Super::ApplyAlgorithm(Data);
	int32 Size = Data->Size();
	for(int32 i = 0; i < Size; i++)
	{
		for(int32 j = 0; j < Size; j++)
		{
			float Value = 0.0f;
			Value = BaseLandscape_Cache.GetValue(i * 0.01f, j * 0.01f, 0.0f) * 2000;
			Data->SetSampleValue(i, j, Value);
		}
	}
}

void UPerlinAlgorithm::DebugAlgorithm(UHITerrainData* Data)
{
	Super::DebugAlgorithm(Data);
	int32 Size = Data->Size();
	for(int32 i = 0; i < Size; i++)
	{
		for(int32 j = 0; j < Size; j++)
		{
			float Value = 0.0f;
			Value = BaseLandscape_Cache.GetValue(i * 0.01f, j * 0.01f, 0.0f) * 2000;
			Data->SetSampleValue(i, j, Value);
		}
	}
}
