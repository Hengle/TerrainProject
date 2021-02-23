#include "HITerrainPerlinGenerator.h"

void HITerrainPerlinGenerator::Init(int32 InSeed) 
{
	HITerrainNoiseGenerator::Init(InSeed);
	Noise.SetSeed(InSeed);
}

float HITerrainPerlinGenerator::GetValue(float X, float Y) {
	if (bInited) 
	{
		return Noise.GetValue(X, Y, 0);
	}
	else 
	{
		return NAN;
	}
}