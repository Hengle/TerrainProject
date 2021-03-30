// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "noiselib/module/perlin.h"
#include "noiselib/module/curve.h"

/*
 * CurvedPerlin数学模块
 * 就是libnoise的Perlin+Curve的一个组合，并且把三维变成了二维。
 * 加入了控制点就会加Curved，否则就是单纯的Perlin。
 */
class HITERRAINMAKER_API FHITerrainCurvedPerlin
{

public:
	void Init(int32 InSeed = 10086, float InFrequency = 1.0f, float InLacunarity = 2.0f, int32 InOctaveCount = 6, float InPersistence = 0.5f);

	void AddControlPoint(float InputValue, float OutputValue);

	float GetValue(float X, float Y);
	
private:
	bool bUseControlPoint = false;
	
	noise::module::Perlin Perlin;
	noise::module::Curve Curve;
};
