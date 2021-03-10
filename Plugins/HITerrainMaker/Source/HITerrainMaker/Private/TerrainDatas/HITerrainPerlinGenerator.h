#pragma once
#include "CoreMinimal.h"
#include "noiselib/noise.h"
#include "HITerrainNoiseGenerator.h"

class HITerrainPerlinGenerator: public HITerrainNoiseGenerator 
{
public:
	virtual void Init(int32 InSeed) override;

	virtual float GetValue(float X, float Y) override;

private:
	noise::module::Perlin Noise;
};