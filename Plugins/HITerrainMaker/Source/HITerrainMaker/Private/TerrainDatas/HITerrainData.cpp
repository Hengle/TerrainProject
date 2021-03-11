#include "HITerrainData.h"

uint32 UHITerrainData::Run()
{
	int32 TotalSize = Size();
	for(int i = 0; i < TotalSize; i++)
	{
		for(int j = 0; j < TotalSize; j++)
		{
			TerrainData.Add(0.0f);
		}
	}
	bIsGenerated = true;
	for(UHITerrainAlgorithm* Algorithm: Algorithms)
	{
		Algorithm->Apply(this);
	}
	OnDataGenerated.ExecuteIfBound();
	return 0;
}

FChunkDataPtr UHITerrainData::GetChunkData(const TPair<int32, int32>& Index)
{
	if (Index.Key < 0 || Index.Key >= ChunkNums || Index.Value < 0 || Index.Value >= ChunkNums) 
	{
		UE_LOG(LogHITerrain, Error, TEXT("UHITerrainDataBase::GetChunkData Out Of Range! [%d, %d]"), Index.Key, Index.Value);
		return nullptr;
	}
	else if(!bIsGenerated)
	{
		UE_LOG(LogHITerrain, Error, TEXT("UHITerrainDataBase::GetChunkData Not Generated!"));
		return nullptr;
	}
	else 
	{
		FChunkDataPtr Data = MakeShared<FHITerrainChunkData, ESPMode::ThreadSafe>();
		Data->Data = this;
		Data->ChunkSize = ChunkSize;
		Data->Index = Index;
		return Data;
	}
}

void UHITerrainData::SetChunkNums(int32 InChunkNums) 
{
	ChunkNums = InChunkNums;
}

void UHITerrainData::SetChunkSize(int32 InChunkSize) 
{
	ChunkSize = InChunkSize;
}

int32 UHITerrainData::GetIndex(int32 X, int32 Y, int32 TotalSize)
{
	return X * TotalSize + Y;
}

void UHITerrainData::ApplyAlgorithm(UHITerrainAlgorithm* Algorithm)
{
	// TODO
}

float UHITerrainData::GetSample(int32 X, int32 Y)
{
	int32 TotalSize = Size();
	if (X < 0 || X >= TotalSize || Y < 0 || Y >= TotalSize)
	{
		UE_LOG(LogHITerrain, Error, TEXT("UHITerrainDataBase::GetSample Out Of Range! [%d, %d]"), X, Y);
		return 0.0f;
	}
	else if (!bIsGenerated)
	{
		UE_LOG(LogHITerrain, Error, TEXT("UHITerrainDataBase::GetSample Not Generated!"));
		return 0.0f;
	}
	else
	{
		return TerrainData[GetIndex(X, Y, TotalSize)];
	}
}

void UHITerrainData::SetSample(int32 X, int32 Y, float Value)
{
	int32 TotalSize = Size();
	if (X < 0 || X >= TotalSize || Y < 0 || Y >= TotalSize)
	{
		UE_LOG(LogHITerrain, Error, TEXT("UHITerrainDataBase::SetSample Out Of Range! [%d, %d]"), X, Y);
	}
	else if (!bIsGenerated)
	{
		UE_LOG(LogHITerrain, Error, TEXT("UHITerrainDataBase::SetSample Not Generated!"));
	}
	else
	{
		TerrainData[GetIndex(X, Y, TotalSize)] = Value;
	}
}

void UHITerrainData::SetAlgorithms(const TArray<UHITerrainAlgorithm*>& InAlgorithms)
{
	Algorithms = InAlgorithms;
}

int32 UHITerrainData::Size()
{
	return ChunkNums * ChunkSize + 1;
}

int32 UHITerrainData::GetChunkNums()
{
	return ChunkNums;
}

int32 UHITerrainData::GetChunkSize()
{
	return ChunkSize;
}
