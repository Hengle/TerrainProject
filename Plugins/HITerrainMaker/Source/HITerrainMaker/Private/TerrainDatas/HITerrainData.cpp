#include "HITerrainData.h"

uint32 UHITerrainData::Run()
{
	int32 TotalSize = Size();
	TerrainData = TFixed2DArray<FTerrainSample>(TotalSize, TotalSize, FTerrainSample());
	bIsGenerated = true;
	for(UHITerrainAlgorithm* Algorithm: Algorithms)
	{
		if(Information->bEnableDebugAlgorithm)
		{
			Algorithm->DebugAlgorithm(this);
		}
		else
		{
			Algorithm->ApplyAlgorithm(this);
		}
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

float UHITerrainData::GetSampleValue(int32 X, int32 Y)
{
	if (!bIsGenerated)
	{
		UE_LOG(LogHITerrain, Error, TEXT("UHITerrainDataBase::GetSample Not Generated!"));
		return 0.0f;
	}
	else
	{
		return TerrainData.GetValue(X, Y).Value;
	}
}

ESampleType UHITerrainData::GetSampleType(int32 X, int32 Y)
{
	if (!bIsGenerated)
	{
		UE_LOG(LogHITerrain, Error, TEXT("UHITerrainDataBase::GetSample Not Generated!"));
		return ESampleType::NONE;
	}
	else
	{
		return TerrainData.GetValue(X, Y).Type;
	}
}

void UHITerrainData::SetSampleValue(int32 X, int32 Y, float Value)
{
	if (!bIsGenerated)
	{
		UE_LOG(LogHITerrain, Error, TEXT("UHITerrainDataBase::SetSample Not Generated!"));
	}
	else
	{
		TerrainData.GetValueRef(X, Y).Value = Value;
	}
}

float UHITerrainData::GetSampleValue(float X, float Y)
{
	// TODO: 搞个插值
	return 0.0f;
}

ESampleType UHITerrainData::GetSampleType(float X, float Y)
{
	// TODO: ESampleType没法插值，看看这里怎么搞，要不就优先级，要不就考虑别的实现
	return ESampleType::NONE;
}

void UHITerrainData::SetSampleType(int32 X, int32 Y, ESampleType Type)
{
	if (!bIsGenerated)
	{
		UE_LOG(LogHITerrain, Error, TEXT("UHITerrainDataBase::SetSample Not Generated!"));
	}
	else
	{
		TerrainData.GetValueRef(X, Y).Type = Type;
	}
}

void UHITerrainData::SetAlgorithms(const TArray<UHITerrainAlgorithm*>& InAlgorithms)
{
	Algorithms = InAlgorithms;
}

void UHITerrainData::SetInformation(FTerrainInformationPtr InInformation)
{
	Information = InInformation;
}

int32 UHITerrainData::Size()
{
	return ChunkNums * ChunkSize + 1;
}

FVector2D UHITerrainData::GetCenterPoint()
{
	return FVector2D(Size() / 2, Size() / 2);
}

int32 UHITerrainData::GetChunkNums()
{
	return ChunkNums;
}

int32 UHITerrainData::GetChunkSize()
{
	return ChunkSize;
}
