// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

class HITERRAINMAKER_API FHITerrainMathMisc
{
public:
	/*
	 * 判断浮点数是否是NaN
	 */
	static FORCEINLINE bool IsNaN(const float& InFloat)
	{
		return FMath::IsNaN(InFloat);
	}

	/*
	 * 判断FVector是否是NaN
	 */
	static FORCEINLINE bool IsNaN(const FVector& InFVector)
	{
		return FMath::IsNaN(InFVector.X) || FMath::IsNaN(InFVector.Y) || FMath::IsNaN(InFVector.Z);
	}

	/*
	 * 判断FQuat是否是NaN
	 */
	static FORCEINLINE bool IsNaN(const FQuat& InFQuat)
	{
		return FMath::IsNaN(InFQuat.X) || FMath::IsNaN(InFQuat.Y) || FMath::IsNaN(InFQuat.Z) || FMath::IsNaN(InFQuat.W);
	}

	/*
	 * 二维SmoothStep插值
	 */
	static FORCEINLINE float Lerp2D(float LL, float LH, float HL, float HH, float LA, float HA)
    {
    	float L1 = Lerp(LL, LH, LA);
    	float L2 = Lerp(HL, HH, LA);
    	return Lerp(L1, L2, HA);
    }

	/*
	 * 二维线性插值
	 */
	static FORCEINLINE float LinearLerp2D(float LL, float LH, float HL, float HH, float LA, float HA)
	{
		float L1 = LinearLerp(LL, LH, LA);
		float L2 = LinearLerp(HL, HH, LA);
		return LinearLerp(L1, L2, HA);
	}

	/*
	 * 一维SmoothStep插值
	 */
    static FORCEINLINE float Lerp(float Low, float High, float Alpha)
    {
    	return Low * (1 - SmoothStep(Alpha)) + High * SmoothStep(Alpha);
    }

	/*
	 * 一维线性插值
	 */
	static FORCEINLINE float LinearLerp(float Low, float High, float Alpha)
	{
		return Low * (1 - Alpha) + High * Alpha;
	}

	/*
	 * SmoothStep函数
	 */
    static FORCEINLINE float SmoothStep(float Alpha)
    {
    	return Alpha * Alpha * (3 - 2 * Alpha);
    }
};
