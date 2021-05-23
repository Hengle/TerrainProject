// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainAlgorithms/EcoSystemAlgorithms/EcoSystemAlgorithm.h"

void UEcoSystemAlgorithm::Init(FTerrainInformationPtr InInformation)
{
	Super::Init(InInformation);
	// Voronoi.SetSizeX(InInformation->ChunkSize);
	// Voronoi.SetSizeY(InInformation->ChunkSize);
	// Voronoi.SetNumSites(1000);
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
	Data->AddChannel("b", ETerrainDataType::FLOAT); // 湿度值
	Data->AddChannel("a", ETerrainDataType::FLOAT);
	// CalculateSlope(Data);
	CalculateUnderWaterTerrain(Data);

	Data->bAvailable = true;
	SlopeGPU.ApplyModule(Data);
	while(!Data->bAvailable)
	{
		FPlatformProcess::Sleep(0.1);
	}
	Data->bAvailable = false;
	
	Data->bAvailable = true;
	HumidityGPU.ApplyModule(Data);
	while(!Data->bAvailable)
	{
		FPlatformProcess::Sleep(0.1);
	}
	Data->bAvailable = false;
	
	// CalculateHumidity(Data);
	int32 ChunkNum = Information->ChunkNum;
	// int32 ChunkNum = 1;
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
	int32 Size = Data->GetChunkSize() * 2;
	float Step = 100.0f / 2;
	float RecentX = Step, RecentY = Step;
	FRandomStream RandomStream(Information->Seed);
	for(int32 i = 0; i < Size - 1; i++)
	{
		for(int32 j = 0; j < Size - 1; j++)
		{
			float LocationX = Index.Key * Information->ChunkSize + RecentX;
			LocationX += RandomStream.FRand() * 25;
			float LocationY = Index.Value * Information->ChunkSize + RecentY;
			LocationY += RandomStream.FRand() * 25;
			float SlopeValue = 1.0f;
			float WaterValue = 0.0f;
			float HumidityValue = 0.0f;
			Data->GetChannelValue("r", LocationX, LocationY, SlopeValue);
			Data->GetChannelValue("g", LocationX, LocationY, WaterValue);
			Data->GetChannelValue("b", LocationX, LocationY, HumidityValue);
			// if(SlopeValue < 0.1f && WaterValue < 0.1f)
			if(HumidityValue > 0.1f)
			{
				float LocationZ = Data->GetHeightValue(LocationX, LocationY) - SlopeValue * 500;
				FVector Location(LocationX, LocationY, LocationZ);
				Data->AddChunkGrass(Index, Location);
			}
			RecentY += Step;
		}
		RecentX += Step;
		RecentY = Step;
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
			float BValue = j == Size - 1? Data->GetHeightValue(i, j) + Data->GetSedimentValue(i, j):
									Data->GetHeightValue(i, j + 1) + Data->GetSedimentValue(i, j + 1);
			float SlopeValue = ((LValue - RValue) * (LValue - RValue) + (TValue - BValue) * (TValue - BValue)) / 40000.0f;
			// Data->SetChannelValue("slope", i, j, SlopeValue);
			SlopeValue = 1.0f - FMath::Clamp(SlopeValue, 0.0f, 1.0f);
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

void UEcoSystemAlgorithm::CalculateHumidity(UHITerrainData* Data)
{
	int32 Size = Data->Size();
	int32 Scope = 5;
	for(int32 i = 0; i < Size; i++)
	{
		for(int32 j = 0; j < Size; j++)
		{
			float SumWaterValue = 0.0f;
			int32 SumGrid = 0;
			for(int32 u = i - Scope; u < i + Scope + 1; u++)
			{
				for(int32 v = j - Scope; v < j + Scope + 1; v++)
				{
					if(u >= 0 && u < Size && v >=0 && v < Size)
					{
						float UVWaterValue = 0.0f;
						Data->GetChannelValue("water", i, j, UVWaterValue);
						SumWaterValue += UVWaterValue;
						SumGrid ++;
					}
				}
			}
			Data->SetChannelValue("b", i, j, SumWaterValue / SumGrid);
		}
	}
}
