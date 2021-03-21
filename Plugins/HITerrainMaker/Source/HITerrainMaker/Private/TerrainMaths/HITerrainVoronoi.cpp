// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainMaths/HITerrainVoronoi.h"

void FHITerrainVoronoi::Init(int32 InSeed, float InFrequency, float InDisplacement, int32 InSize, float InScale)
{
	Seed = InSeed;
	Frequency = InFrequency;
	Displacement = InDisplacement;
	Size = InSize;
	Scale = InScale;
	for(int32 i = 0; i < Size; i++)
	{
		for(int32 j = 0; j < Size; j++)
		{
			Samples = T2DArray<FVoronoiSample>(Size, Size, FVoronoiSample());
		}
	}
}

const FVoronoiSample& FHITerrainVoronoi::GetSample(int X, int Y)
{
	return Samples.GetValue(X, Y);
}

/*
 * 改了NoiseLib的Voronoi算法
 */
void FHITerrainVoronoi::GenerateSample(int32 X, int32 Y)
{
	// This method could be more efficient by caching the seed values.  Fix
	// later.
	FVector2D Position(X, Y);
	// x *= m_frequency;
	// y *= m_frequency;
	// z *= m_frequency;
	Position *= Frequency;
	Position *= Scale;

	int XInt = (Position.X > 0.0? (int)Position.X: (int)Position.X - 1);
	int YInt = (Position.Y > 0.0? (int)Position.Y: (int)Position.Y - 1);

	double MinDist = 2147483647.0;
	double XCandidate = 0;
	double YCandidate = 0;
	double ZCandidate = 0;

	// Inside each unit cube, there is a seed point at a random position.  Go
	// through each of the nearby cubes until we find a cube with a seed point
	// that is closest to the specified position.
	for (int ZCur = 0 - 2; ZCur <= 0 + 2; ZCur++) {
		for (int YCur = YInt - 2; YCur <= YInt + 2; YCur++) {
			for (int XCur = XInt - 2; XCur <= XInt + 2; XCur++) {
				// Calculate the position and distance to the seed point inside of
				// this unit cube.
				double XPos = XCur + noise::ValueNoise3D (XCur, YCur, ZCur, Seed    );
				double YPos = YCur + noise::ValueNoise3D (XCur, YCur, ZCur, Seed + 1);
				double ZPos = ZCur + noise::ValueNoise3D (XCur, YCur, ZCur, Seed + 2);
				double XDist = XPos - X;
				double YDist = YPos - Y;
				double ZDist = ZPos - 0.0;
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

	float Value;
	// if (m_enableDistance) {
	// 	// Determine the distance to the nearest seed point.
	// 	double xDist = XCandidate - x;
	// 	double yDist = YCandidate - y;
	// 	double zDist = ZCandidate - z;
	// 	Value = (sqrt (xDist * xDist + yDist * yDist + zDist * zDist)
	// 	  ) * SQRT_3 - 1.0;
	// } else {
	// 	Value = 0.0;
	// }
	Value = 0.0;
	Value += (Displacement * (float)noise::ValueNoise3D ((int32)(floor (XCandidate)),(int32)(floor (YCandidate)),(int32)(floor (ZCandidate))));

	FVoronoiSample Sample;
	Sample.Value = Value;
	Sample.IndexPoint = FVector2D(XCandidate, YCandidate);
	if(!IndexPoints.Contains(Sample.IndexPoint))
	{
		IndexPoints.Add(Sample.IndexPoint);
	}
	// Return the calculated distance with the displacement value applied.
	// return Value + (Displacement * (double)noise::ValueNoise3D (
	// 	(int)(floor (XCandidate)),
	// 	(int)(floor (YCandidate)),
	// 	(int)(floor (ZCandidate))));
	Samples.SetValue(X, Y, Sample);
}


