// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "HITerrainMathMisc.generated.h"

/**
 * 
 */
UCLASS()
class HITERRAINMAKER_API UHITerrainMathMisc : public UObject
{
	GENERATED_BODY()

public:
	static float Lerp2D(float LL, float LH, float HL, float HH, float LA, float HA)
    {
    	float L1 = Lerp(LL, LH, LA);
    	float L2 = Lerp(HL, HH, LA);
    	return Lerp(L1, L2, HA);
    }

	static float LinearLerp2D(float LL, float LH, float HL, float HH, float LA, float HA)
	{
		float L1 = LinearLerp(LL, LH, LA);
		float L2 = LinearLerp(HL, HH, LA);
		return LinearLerp(L1, L2, HA);
	}

    static float Lerp(float Low, float High, float Alpha)
    {
    	return Low * (1 - SmoothStep(Alpha)) + High * SmoothStep(Alpha);
    }

	static float LinearLerp(float Low, float High, float Alpha)
	{
		return Low * (1 - Alpha) + High * Alpha;
	}

    static float SmoothStep(float Alpha)
    {
    	return Alpha * Alpha * (3 - 2 * Alpha);
    }
};
