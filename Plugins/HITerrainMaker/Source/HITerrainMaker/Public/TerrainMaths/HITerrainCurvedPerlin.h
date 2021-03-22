// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "noiselib/module/perlin.h"
#include "noiselib/module/curve.h"

class HITERRAINMAKER_API FHITerrainCurvedPerlin
{

public:
	void Init(int32 InSeed = 10086, float InFrequency = 1.0f, float InLacunarity = 2.0f, int32 InOctaveCount = 6, float InPersistence = 0.5f);

	void AddControlPoint(float InputValue, float OutputValue);

	float GetValue(float X, float Y);
	
private:
	noise::module::Perlin Perlin;
	noise::module::Curve Curve;
};
