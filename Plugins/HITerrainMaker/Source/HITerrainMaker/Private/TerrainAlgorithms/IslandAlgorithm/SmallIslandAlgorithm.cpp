// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainAlgorithms/IslandAlgorithm/SmallIslandAlgorithm.h"

#include "TerrainDatas/HITerrainData.h"

void USmallIslandAlgorithm::Init(FTerrainInformationPtr InInformation)
{
	Super::Init(InInformation);
	int32 Size = Information->ChunkNum * Information->ChunkSize / 100;
	Voronoi.Init(Information->Seed, Size, Size, 200);
	AllCells = Voronoi.GetAllCells();
	Perlin.Init(Information->Seed, 0.2f, 2.0f, 14);
	Perlin.AddControlPoint(-2.0f, -0.5f);
	Perlin.AddControlPoint(-1.0f, -0.5f);
	Perlin.AddControlPoint(0.0f, 0.375f);
	Perlin.AddControlPoint(1.0f, 2.375f);
	Perlin.AddControlPoint(2.0f, 5.625f);
}

void USmallIslandAlgorithm::Apply(UHITerrainData* Data)
{
	Super::Apply(Data);
	int32 Size = Data->Size();
	for(int32 i = 0; i < Size; i++)
	{
		for(int32 j = 0; j < Size; j++)
		{
			
		}
	}
}

void USmallIslandAlgorithm::DebugApply(UHITerrainData* Data)
{
	Super::DebugApply(Data);
	int32 Size = Data->Size();
	GenerateOcean();
	for(int32 i = 0; i < Size; i++)
	{
		for(int32 j = 0; j < Size; j++)
		{
			float Value = 0.0f;
			int32 Cell = Voronoi.Position2Cell(i, j);
			// if(CellTypes.Contains(Cell) && CellTypes[Cell] == ESampleType::OCEAN)
			// {
			// 	Value = -500.0f;
			// }

			Value = Perlin.GetValue(i * 0.01f, j * 0.01f) * 1000;
			if(CellTypes.Contains(Cell))
			{
				if(CellTypes[Cell] == ESampleType::OCEAN)
				{
					Value = -1000.0f;
				}
			}
			// else if(Value < 0.0f)
			// {
			// 	Value = -1000.0f;
			// }
			Data->SetSampleValue(i, j, Value);
		}
	}
}

void USmallIslandAlgorithm::GenerateOcean()
{
	/*
	* -1、-2、-3、-4四个值表示四边的边界，如果Neighbors中有这个值那么说明是紧挨着四边的格子。
	*/
	for(int i = 0; i < AllCells.Num(); i++)
	{
		FVoronoiCellInfo Cell = AllCells[i];
		if(Cell.Neighbors.Contains(-1) || Cell.Neighbors.Contains(-2) || Cell.Neighbors.Contains(-3) || Cell.Neighbors.Contains(-4))
		{
			CellTypes.Add(i, ESampleType::OCEAN);
		}
	}
	for(const FVector& Site: Voronoi.GetSites())
	{
		if(Perlin.GetValue(Site.X * 0.01f, Site.Y * 0.01f) < 0.0f)
		{
			CellTypes.Add(Voronoi.Position2Cell(Site), ESampleType::OCEAN);
		}
	}
}

void USmallIslandAlgorithm::GenerateMainIsland()
{
}
