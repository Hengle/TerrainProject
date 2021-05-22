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
	while(!Data->bAvailable)
	{
		FPlatformProcess::Sleep(0.1);
	}
	Data->bAvailable = false;
	
	Data->AddChannel("r", ETerrainDataType::FLOAT);	// 坡度值
	Data->AddChannel("g", ETerrainDataType::FLOAT);
	Data->AddChannel("b", ETerrainDataType::FLOAT);
	Data->AddChannel("a", ETerrainDataType::FLOAT);
	CalculateSlope(Data);
	CalculateUnderWaterTerrain(Data);
	int32 ChunkNum = Information->ChunkNum;
	for(int32 i = 0; i < ChunkNum; i++)
	{
		for(int32 j = 0; j < ChunkNum; j++)
		{
			TPair<int32, int32> Index(i, j);
			// GenerateChunkGrassData(Data, Index);
		}
	}
	
	Data->bAvailable = true;
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
		float Slope = 0.0f;
		Data->GetChannelValue("r", LocationX, LocationY, Slope);
		if(Slope > 0.5f)
		{
			FVector GrassLocation(LocationX, LocationY, LocationZ);
			Data->AddChunkGrass(Index, GrassLocation);
		}
	}
}

void UEcoSystemAlgorithm::CalculateSlope(UHITerrainData* Data)
{
	int32 Size = Data->Size();
	for(int32 i = 0; i < Size; i++)
	{
		for(int32 j = 0; j < Size; j++)
		{
			float LValue = i == 0? Data->GetHeightValue(i, j) + Data->GetSedimentValue(i, j):
									Data->GetHeightValue(i - 1, j) + Data->GetSedimentValue(i - 1, j);
			float RValue = i == Size - 1? Data->GetHeightValue(i, j) + Data->GetSedimentValue(i, j):
											Data->GetHeightValue(i + 1, j) + Data->GetSedimentValue(i + 1, j);
			float TValue = j == 0? Data->GetHeightValue(i, j) + Data->GetSedimentValue(i, j):
									Data->GetHeightValue(i, j - 1) + Data->GetSedimentValue(i, j - 1);
			float BValue = i == Size - 1? Data->GetHeightValue(i, j) + Data->GetSedimentValue(i, j):
									Data->GetHeightValue(i, j + 1) + Data->GetSedimentValue(i, j + 1);
			float SlopeValue = ((LValue - RValue) * (LValue - RValue) + (TValue - BValue) * (TValue - BValue)) / 40000.0f;
			// Data->SetChannelValue("slope", i, j, SlopeValue);
			Data->SetChannelValue("r", i, j, SlopeValue);
		}
	}
}

void UEcoSystemAlgorithm::CalculateUnderWaterTerrain(UHITerrainData* Data)
{
	int32 Size = Data->Size();
	for(int32 i = 0; i < Size; i++)
	{
		for(int32 j = 0; j < Size; j++)
		{
			float WaterValue;
			Data->GetChannelValue("water", i, j, WaterValue);
			Data->SetChannelValue("g", i, j, WaterValue);
		}
	}
}
