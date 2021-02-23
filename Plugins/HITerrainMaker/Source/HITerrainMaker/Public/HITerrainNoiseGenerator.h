#pragma once
#include "CoreMinimal.h"
#include "HITerrainCommons.h"
#include "noiselib/noise.h"

/**
 * 噪声生成器基类
 */
class HITERRAINMAKER_API HITerrainNoiseGenerator
{
public:
	virtual void Init(int32 InSeed);

	virtual float GetValue(float X, float Y) = 0;

	virtual ~HITerrainNoiseGenerator() {

	};

protected:
	bool bInited = false;
	int32 Seed;
};