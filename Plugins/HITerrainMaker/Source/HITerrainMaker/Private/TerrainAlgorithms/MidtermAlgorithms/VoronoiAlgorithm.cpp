// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainAlgorithms/MidtermAlgorithms/VoronoiAlgorithm.h"

#include "TerrainDatas/HITerrainData.h"

void UVoronoiAlgorithm::Init(FTerrainInformationPtr InInformation)
{
	Super::Init(InInformation);
	int32 Size = Information->ChunkNum * Information->ChunkSize / 100;
	Voronoi.Init(Information->Seed, Size, Size, Information->IG_VoronoiCount);
	AllCells = Voronoi.GetAllCells();
}

void UVoronoiAlgorithm::ApplyAlgorithm(UHITerrainData* Data)
{
	Super::ApplyAlgorithm(Data);
	int32 Size = Data->Size();
	for(int32 i = 0; i < Size; i++)
	{
		for(int32 j = 0; j < Size; j++)
		{
			float Value = 0.0f;
			int32 Cell = Voronoi.Position2Cell(i, j);
			Value = Voronoi.GetCellValue(i, j) * 1000;
			Data->SetHeightValue(i, j, Value);
		}
	}
}

void UVoronoiAlgorithm::DebugAlgorithm(UHITerrainData* Data)
{
	Super::DebugAlgorithm(Data);
	int32 Size = Data->Size();
	for(int32 i = 0; i < Size; i++)
	{
		for(int32 j = 0; j < Size; j++)
		{
			float Value = 0.0f;
			int32 Cell = Voronoi.Position2Cell(i, j);
			Value = Voronoi.GetCellValue(i, j) * 1000;
			Data->SetHeightValue(i, j, Value);
		}
	}
}
