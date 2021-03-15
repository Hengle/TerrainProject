#pragma once
#include "CoreMinimal.h"
#include "TerrainMaths/noiselib/noise.h"
#include "HITerrainNoiseGenerator.h"
#include "TerrainMaths/noiselib/module/perlin.h"

class HITerrainPerlinGenerator: public HITerrainNoiseGenerator 
{
public:
	virtual void Init(int32 InSeed) override;

	virtual float GetValue(float X, float Y) override;

private:
	noise::module::Perlin Noise;
};
