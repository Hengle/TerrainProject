// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainAlgorithms/IslandAlgorithm/EcoSystemAlgorithm.h"

#include "TerrainDatas/HITerrainData.h"

void UEcoSystemAlgorithm::Init(FTerrainInformationPtr InInformation)
{
	Super::Init(InInformation);
	Voronoi.Init(Information->Seed, 1.0f, 1.0f, 0.01f);
	Perlin.SetSeed(Information->Seed + 100);
	Perlin.SetFrequency(1.0);
	Perlin.SetLacunarity(2.0);
	Beach.SetSeed(Information->Seed + 101);
	Beach.SetFrequency(0.1);
	Beach.SetLacunarity(2.0);
	SmallIsland.SetSeed(Information->Seed + 102);
	SmallIsland.SetFrequency(2.0);
	SmallIsland.SetLacunarity(0.5);
	SmallIsland.SetOctaveCount(4);
}

void UEcoSystemAlgorithm::Apply(UHITerrainData* Data)
{
	Super::Apply(Data);
	int32 Size = Data->Size();
	Voronoi.GenerateSamples(Size);
	TSet<FVector2D> SeaIndexPoints;
	for(const FVector2D& IndexPoint: Voronoi.GetIndexPoints())
	{
		if(IndexPoint.X < 2 || IndexPoint.Y < 2 || IndexPoint.X > 8 || IndexPoint.Y > 8)
		{
			SeaIndexPoints.Add(IndexPoint);
		}
	}
	for(int32 i = 0; i < Size; i++)
	{
		for(int32 j = 0; j < Size; j++)
		{
			// float Value = Voronoi.GetValue(i * 0.01f, j * 0.01f, 0) * 1000;
			FVoronoiSample Sample = Voronoi.GetSample(i, j);
			float Value = Sample.Value * 1000;
			if(SeaIndexPoints.Contains(Sample.IndexPoint))
			{
				Data->SetSampleType(i, j, ESampleType::SEA);
			}
			else
			{
					
			}
			Data->SetSampleValue(i, j, Value);
		}
	}
}

void UEcoSystemAlgorithm::DebugApply(UHITerrainData* Data)
{
	Super::DebugApply(Data);
	int32 Size = Data->Size();
	Voronoi.GenerateSamples(Size);
	TSet<FVector2D> SeaIndexPoints;
	for(const FVector2D& IndexPoint: Voronoi.GetIndexPoints())
	{
		if(IndexPoint.X < 2 || IndexPoint.Y < 2 || IndexPoint.X > 8 || IndexPoint.Y > 8)
		{
			SeaIndexPoints.Add(IndexPoint);
		}
	}
	for(int32 i = 0; i < Size; i++)
	{
		for(int32 j = 0; j < Size; j++)
		{
			FVoronoiSample Sample = Voronoi.GetSample(i, j);
			float Value = 0.0f;
			FVector2D CenterPoint = Data->GetCenterPoint();
			float XDistance = FMath::Abs(CenterPoint.X - i);
			float YDistance = FMath::Abs(CenterPoint.Y - j);
			float MaxDistance = UE_SQRT_2 * Information->ChunkSize * Information->ChunkNum / 200;
			float Alpha = (MaxDistance - FMath::Sqrt(XDistance * XDistance + YDistance * YDistance)) / MaxDistance;
			if(SeaIndexPoints.Contains(Sample.IndexPoint))
			{
				float SmallIslandValue = SmallIsland.GetValue(i * 0.01f, j * 0.01f, 0);
				if(SmallIslandValue > 0.3)
				{
					Value = -500.0f;
				}
			}
			else
			{
				float PerlinValue = Perlin.GetValue(i * 0.01f, j * 0.01f, 0) * 1.5;
				PerlinValue -= (1 - Alpha) * 0.5;
				if(PerlinValue > 0)
				{
					Value = PerlinValue * 2000;
				}
				if(Value < 500.0f)
				{
					Value = 500.0f;
				}
			}
			Data->SetSampleValue(i, j, Value);
		}
	}
}
