#pragma once

#include "CoreMinimal.h"

#include "HITerrainModule.h"
#include "TerrainMaths/noiselib/module/module.h"

/**
 * 
 */
class HITERRAINMAKER_API FHITerrainPerlin: public FHITerrainModule
{
public:
	FHITerrainPerlin();
	~FHITerrainPerlin();

	void SetTargetChannel(const FString& InChannelName);
	void SetScale(float InScale);
	void SetAmplitude(float InAmplitude);
	void SetSeed(int32 InSeed);
	void SetFrequency(float InFrequency);
	void SetLacunarity(float InLacunarity);
	void SetPersistence(float InPersistence);
	void SetOctaveCount(float InOctaveCount);

	float GetValue(float X, float Y);

	virtual void ApplyModule(UHITerrainData* Data) override;

private:
	noise::module::Perlin Perlin;

	FString ChannelName;
	float Scale;
	float Amplitude;
	int32 Seed;
	float Frequency;
	float Lacunarity;
	float Persistence;
	float OctaveCount;
};
