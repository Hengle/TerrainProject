// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainMaths/Modules/HITerrainPerlin.h"


FHITerrainPerlin::FHITerrainPerlin()
{
}

FHITerrainPerlin::~FHITerrainPerlin()
{
}

void FHITerrainPerlin::SetTargetChannel(const FString& InChannelName)
{
	ChannelName = InChannelName;
}

void FHITerrainPerlin::SetScale(float InScale)
{
	Scale = InScale;
}

void FHITerrainPerlin::SetAmplitude(float InAmplitude)
{
	Amplitude = InAmplitude;
}

void FHITerrainPerlin::SetSeed(int32 InSeed)
{
	Seed = InSeed;
	Perlin.SetSeed(Seed);
}

void FHITerrainPerlin::SetFrequency(float InFrequency)
{
	Frequency = InFrequency;
	Perlin.SetFrequency(Frequency);
}

void FHITerrainPerlin::SetLacunarity(float InLacunarity)
{
	Lacunarity = InLacunarity;
	Perlin.SetLacunarity(Lacunarity);
}

void FHITerrainPerlin::SetPersistence(float InPersistence)
{
	Persistence = InPersistence;
	Perlin.SetPersistence(Persistence);
}

void FHITerrainPerlin::SetOctaveCount(float InOctaveCount)
{
	OctaveCount = InOctaveCount;
	Perlin.SetOctaveCount(OctaveCount);
}

void FHITerrainPerlin::ApplyModule(UHITerrainData* Data)
{
	Data->AddChannel(ChannelName, ETerrainDataType::FLOAT);
	auto Channel = Data->GetChannel(ChannelName);
	for(int32 i = 0; i < Channel->GetSizeX(); i++)
	{
		for(int32 j = 0; j < Channel->GetSizeY(); j++)
		{
			float Value = Perlin.GetValue(i * Scale, j * Scale, 0) * Amplitude;
			Channel->SetChannelValue(i, j, Value);
		}
	}
}
