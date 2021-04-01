// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "noiselib/module/perlin.h"
#include "HITerrainModule.h"

/**
 * 
 */
class HITERRAINMAKER_API FHITerrainPerlin: FHITerrainModule
{
public:
	void SetSeed(int32 Seed);
	void SetFrequency(float Frequency);
	void SetLacunarity(float Lacunarity);
	void SetOctaveCount (int32 OctaveCount);
	void SetPersistence (float Persistence);
	void SetNoiseQuality (noise::NoiseQuality NoiseQuality);
	
public:
	virtual float GetValue(float X, float Y) override;
	
	virtual int32 GetSourceModuleCapacity() override;

	FHITerrainPerlin();

private:
	noise::module::Perlin Perlin;
};
