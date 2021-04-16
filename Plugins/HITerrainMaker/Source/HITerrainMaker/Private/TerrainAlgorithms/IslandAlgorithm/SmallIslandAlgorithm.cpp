/*
* This is an independent project of an individual developer. Dear PVS-Studio, please check it.
* PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
*/
// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainAlgorithms/IslandAlgorithm/SmallIslandAlgorithm.h"

#include "TerrainDatas/HITerrainData.h"

void USmallIslandAlgorithm::Init(FTerrainInformationPtr InInformation)
{
	Super::Init(InInformation);
	int32 Size = Information->ChunkNum * Information->ChunkSize / 100;
	Voronoi.Init(Information->Seed, Size, Size, Information->IG_VoronoiCount);
	AllCells = Voronoi.GetAllCells();
	Perlin.Init(Information->Seed, 0.2f, 2.0f, 14);
	Perlin.AddControlPoint(-1.0f, -1.0f);
	Perlin.AddControlPoint(-0.5f, -0.5f);
	Perlin.AddControlPoint(0.0f, 0.0f);
	Perlin.AddControlPoint(0.5f, 1.0f);
	Perlin.AddControlPoint(0.7f, 5.375f);
	Perlin.AddControlPoint(1.0f, 12.625f);
}

void USmallIslandAlgorithm::ApplyAlgorithm(UHITerrainData* Data)
{
	Super::ApplyAlgorithm(Data);
	int32 Size = Data->Size();
	GenerateOcean();
	for(int32 i = 0; i < Size; i++)
	{
		for(int32 j = 0; j < Size; j++)
		{
			float Value = 0.0f;
			int32 Cell = Voronoi.Position2Cell(i, j);
			Value = Perlin.GetValue(i * 0.01f, j * 0.01f);
			if(CellTypes.Contains(Cell))
			{
				if(CellTypes[Cell] == ESampleType::OCEAN)
				{
					Value = -1000.0f;
				}
				else if(CellTypes[Cell] == ESampleType::MARK_WATER)
				{
					Value = 0.0f;
				}
				else if(CellTypes[Cell] == ESampleType::MARK_NEARWATER)
				{
					Value *= 500.0f;
				}
			}
			else
			{
				Value *= 1000.0f;
			}
			Data->SetHeightValue(i, j, Value);
		}
	}
}

void USmallIslandAlgorithm::DebugAlgorithm(UHITerrainData* Data)
{
	Super::DebugAlgorithm(Data);
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

			// Value = Perlin.GetValue(i * 0.01f, j * 0.01f) * 1000;
			Value = Voronoi.GetCellValue(i * 0.01f, j * 0.01f) * 1000;
			// if(CellTypes.Contains(Cell))
			// {
			// 	if(CellTypes[Cell] == ESampleType::OCEAN)
			// 	{
			// 		Value = -1000.0f;
			// 	}
			// 	else if(CellTypes[Cell] == ESampleType::MARK_WATER)
			// 	{
			// 		Value = 1000.0f;
			// 	}
			// 	else if(CellTypes[Cell] == ESampleType::MARK_NEARWATER)
			// 	{
			// 		Value = 2000.0f;
			// 	}
			// }
			
			// else if(Value < 0.0f)
			// {
			// 	Value = -1000.0f;
			// }
			Data->SetHeightValue(i, j, Value);
		}
	}
}

void USmallIslandAlgorithm::GenerateOcean()
{
	int32 OceanRoot = -1;	// 海洋的根节点，用这个到后面做洪泛
	/*
	* -1、-2、-3、-4四个值表示四边的边界，如果Neighbors中有这个值那么说明是紧挨着四边的格子。
	* 这里是生成最外圈的海洋，保证最外圈的维诺图格子没有非海洋的情况
	*/
	for(int i = 0; i < AllCells.Num(); i++)
	{
		FVoronoiCellInfo Cell = AllCells[i];
		if(Cell.Neighbors.Contains(-1) || Cell.Neighbors.Contains(-2) || Cell.Neighbors.Contains(-3) || Cell.Neighbors.Contains(-4))
		{
			CellTypes.Add(i, ESampleType::OCEAN);
			if(OceanRoot == -1)	// 如果等于-1，就记录一下根节点
			{
				OceanRoot = i;
			}
		}
	}
	/*
	 * 根据Perlin来生成水域
	 */
	for(const FVector& Site: Voronoi.GetSites())
	{
		if(Perlin.GetValue(Site.X * 0.01f, Site.Y * 0.01f) < 0.0f)
		{
			if(!CellTypes.Contains(Voronoi.Position2Cell(Site)))
			{
				CellTypes.Add(Voronoi.Position2Cell(Site), ESampleType::MARK_WATER);
			}
		}
	}
	/*
	 * 做洪泛，从而标记海洋
	 */
	TSet<int32> OceanSet;
	OceanSet.Add(OceanRoot);
	FloodOcean(OceanRoot, OceanSet);
	/*
	 * 标记海边的格子
	 */
	for(const FVector& Site: Voronoi.GetSites())
	{
		int32 Cell = Voronoi.Position2Cell(Site);
		if(!CellTypes.Contains(Cell))
		{
			for(int32 Neighbor: AllCells[Cell].Neighbors)
			{
				if(CellTypes.Contains(Neighbor) && CellTypes[Neighbor] == ESampleType::OCEAN)
				{
					CellTypes.Add(Cell, ESampleType::MARK_NEARWATER);
					break;
				}
			}
		}
	}
}

void USmallIslandAlgorithm::FloodOcean(int32 Root, TSet<int32>& OceanSet)
{
	if(CellTypes.Contains(Root) && CellTypes[Root] == ESampleType::OCEAN)
	{
		for(int32 Neighbor: AllCells[Root].Neighbors)
		{
			if(!OceanSet.Contains(Neighbor) && CellTypes.Contains(Neighbor) && (CellTypes[Neighbor] == ESampleType::MARK_WATER || CellTypes[Neighbor] == ESampleType::OCEAN))
			{
				OceanSet.Add(Neighbor);
				CellTypes[Neighbor] = ESampleType::OCEAN;
				for(int32 MoreNeighbor: AllCells[Neighbor].Neighbors)
				{
					FloodOcean(MoreNeighbor, OceanSet);
				}
			}
		}
	}
	
}

void USmallIslandAlgorithm::GenerateMainIsland()
{
}
