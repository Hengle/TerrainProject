// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainAlgorithms/EcoSystemAlgorithms/EcoSystemAlgorithm.h"

void UEcoSystemAlgorithm::Init(FTerrainInformationPtr InInformation)
{
	Super::Init(InInformation);
	Voronoi.SetSizeX(InInformation->ChunkSize);
	Voronoi.SetSizeY(InInformation->ChunkSize);
	Voronoi.SetNumSites(1000);
}

void UEcoSystemAlgorithm::ApplyAlgorithm(UHITerrainData* Data)
{
	Data->Mutex.Lock();
	Super::ApplyAlgorithm(Data);
	int32 ChunkNum = Information->ChunkNum;
	for(int32 i = 0; i < ChunkNum; i++)
	{
		for(int32 j = 0; j < ChunkNum; j++)
		{
			TPair<int32, int32> Index(i, j);
			GenerateChunkGrassData(Data, Index);
		}
	}
	Data->Mutex.Unlock();
}

void UEcoSystemAlgorithm::DebugAlgorithm(UHITerrainData* Data)
{
	Super::DebugAlgorithm(Data);
	// Data->Mutex.Lock();
	// int32 ChunkNum = Information->ChunkNum;
	// for(int32 i = 0; i < ChunkNum; i++)
	// {
	// 	for(int32 j = 0; j < ChunkNum; j++)
	// 	{
	// 		TPair<int32, int32> Index(i, j);
	// 		GenerateChunkGrassData(Data, Index);
	// 	}
	// }
	// Data->Mutex.Unlock();
	Data->AddChannel("R", ETerrainDataType::FLOAT);
	Data->AddChannel("G", ETerrainDataType::FLOAT);
	Data->AddChannel("B", ETerrainDataType::FLOAT);
	Data->AddChannel("A", ETerrainDataType::FLOAT);
}

void UEcoSystemAlgorithm::GenerateChunkGrassData(UHITerrainData* Data, TPair<int32, int32>& Index)
{
	Voronoi.SetSeed(Information->Seed + Information->ChunkNum * Index.Key + Index.Value);
	Voronoi.ApplyModule(Data);
	auto Sites = Voronoi.GetSites();
	for(const FVector& Site: Sites)
	{
		float LocationX = Information->ChunkSize * Index.Key + Site.X;
		float LocationY = Information->ChunkSize * Index.Key + Site.Y;
		float LocationZ = Data->GetHeightValue(LocationX, LocationY);
		float Sediment = Data->GetSedimentValue(LocationX, LocationY);
		if(Sediment > 0.5f)
		{
			FVector GrassLocation(LocationX, LocationY, LocationZ);
			Data->AddChunkGrass(Index, GrassLocation);
		}
	}
}
