// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainMaths/HITerrainVoronoi.h"

void FHITerrainVoronoi::Init(int32 InSeed, float InFrequency, float InDisplacement, float InScale)
{
	Seed = InSeed;
	Frequency = InFrequency;
	Displacement = InDisplacement;
	Scale = InScale;
}

void FHITerrainVoronoi::GenerateSamples(int32 InSize)
{
	Size = InSize;
	Samples = T2DArray<FVoronoiSample>(Size, Size, FVoronoiSample());
	for(int32 i = 0; i < Size; i++)
	{
		for(int32 j = 0; j < Size; j++)
		{
			GenerateSample(i, j);
		}
	}
}


const FVoronoiSample& FHITerrainVoronoi::GetSample(int X, int Y)
{
	return Samples.GetValue(X, Y);
}

const TSet<FVector2D>& FHITerrainVoronoi::GetIndexPoints()
{
	return IndexPoints;
}

/*
 * 改了NoiseLib的Voronoi算法
 */
void FHITerrainVoronoi::GenerateSample(int32 X, int32 Y)
{
	// This method could be more efficient by caching the seed values.  Fix
	// later.

	double Xf = (double)X * Frequency * Scale;
	double Yf = (double)Y * Frequency * Scale;
	double Zf = 0.0f;

	int XInt = (Xf > 0.0? (int)Xf: (int)Xf - 1);
	int YInt = (Yf > 0.0? (int)Yf: (int)Yf - 1);
	int ZInt = (Zf > 0.0? (int)Zf: (int)Zf - 1);

	double MinDist = 2147483647.0;
	double XCandidate = 0;
	double YCandidate = 0;
	double ZCandidate = 0;

	// Inside each unit cube, there is a seed point at a random position.  Go
	// through each of the nearby cubes until we find a cube with a seed point
	// that is closest to the specified position.
	for (int ZCur = ZInt - 2; ZCur <= ZInt + 2; ZCur++) {
		for (int YCur = YInt - 2; YCur <= YInt + 2; YCur++) {
			for (int XCur = XInt - 2; XCur <= XInt + 2; XCur++) {

				// Calculate the position and distance to the seed point inside of
				// this unit cube.
				double XPos = XCur + noise::ValueNoise3D (XCur, YCur, ZCur, Seed    );
				double YPos = YCur + noise::ValueNoise3D (XCur, YCur, ZCur, Seed + 1);
				double ZPos = ZCur + noise::ValueNoise3D (XCur, YCur, ZCur, Seed + 2);
				double XDist = XPos - Xf;
				double YDist = YPos - Yf;
				double ZDist = ZPos - Zf;
				double Dist = XDist * XDist + YDist * YDist + ZDist * ZDist;

				if (Dist < MinDist) {
					// This seed point is closer to any others found so far, so record
					// this seed point.
					MinDist = Dist;
					XCandidate = XPos;
					YCandidate = YPos;
					ZCandidate = ZPos;
				}
			}
		}
	}

	double Value = (Displacement * (double)noise::ValueNoise3D (
	    (int)(floor (XCandidate)),
	    (int)(floor (YCandidate)),
	    (int)(floor (ZCandidate))));
	
	FVoronoiSample Sample;
	Sample.Value = Value;
	Sample.IndexPoint = FVector2D(XCandidate, YCandidate);
	IndexPoints.Add(Sample.IndexPoint);
	Samples.SetValue(X, Y, Sample);
}


