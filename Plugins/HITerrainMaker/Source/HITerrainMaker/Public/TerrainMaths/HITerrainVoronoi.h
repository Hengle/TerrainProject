// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TerrainMaths/noiselib/module/voronoi.h"
#include "TerrainMaths/2DArray.h"

struct FVoronoiSample
{
	float Value;
	FVector2D IndexPoint;
};

/**
 * 
 */
class HITERRAINMAKER_API FHITerrainVoronoi
{
public:
	void Init(int32 InSeed, float InFrequency, float InDisplacement, int32 InSize, float InScale);

private:
	void GenerateSample(int32 X, int32 Y);
	
private:
	int32 Seed;
	float Frequency;
	float Displacement;
	int32 Size;
	float Scale;
	T2DArray<FVoronoiSample> Samples;
	TSet<FVector2D> IndexPoints;
	
	noise::module::Voronoi Voronoi;
};
