// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainMaths/HITerrainPerlin.h"

void FHITerrainPerlin::SetSeed(int32 Seed)
{
	Perlin.SetSeed(Seed);
}

void FHITerrainPerlin::SetFrequency(float Frequency)
{
	Perlin.SetFrequency(Frequency);
}

void FHITerrainPerlin::SetLacunarity(float Lacunarity)
{
	Perlin.SetLacunarity(Lacunarity);
}

void FHITerrainPerlin::SetOctaveCount(int32 OctaveCount)
{
	Perlin.SetOctaveCount(OctaveCount);
}

void FHITerrainPerlin::SetPersistence(float Persistence)
{
	Perlin.SetPersistence(Persistence);
}

void FHITerrainPerlin::SetNoiseQuality(noise::NoiseQuality NoiseQuality)
{
	Perlin.SetNoiseQuality(NoiseQuality);
}

FHITerrainPerlin::FHITerrainPerlin()
{
}

float FHITerrainPerlin::GetValue(float X, float Y)
{
	return Perlin.GetValue(X, Y, 0.0f);
}

int32 FHITerrainPerlin::GetSourceModuleCapacity()
{
	return 0;
}
