#include "HITerrainDataBase.h"

FChunkInformationPtr UHITerrainDataBase::GetChunkData(const TPair<int32, int32>& Index)
{
	if (Index.Key < 0 || Index.Key >= ChunkNums || Index.Value < 0 || Index.Value >= ChunkNums) 
	{
		UE_LOG(LOGHITerrain, Error, TEXT("UHITerrainDataBase::GetChunkData Out Of Range! [%d, %d]"), Index.Key, Index.Value);
		return nullptr;
	}
	else if(!bIsGenerated)
	{
		UE_LOG(LOGHITerrain, Error, TEXT("UHITerrainDataBase::GetChunkData Not Generated!"));
		return nullptr;
	}
	else 
	{
		return ChunkData[Index];
	}
}

void UHITerrainDataBase::SetSeed(int32 InSeed)
{
	Seed = InSeed;
}

void UHITerrainDataBase::SetChunkNums(int32 InChunkNums) 
{
	ChunkNums = InChunkNums;
}

void UHITerrainDataBase::SetChunkSampleNums(int32 InChunkSampleNums) 
{
	ChunkSampleNums = InChunkSampleNums;
}

float UHITerrainDataBase::GetSample(int32 X, int32 Y)
{
	int32 ChunkTotalSize = ChunkNums * (ChunkSampleNums + 1);
	if (X < 0 || X >= ChunkTotalSize || Y < 0 || Y >= ChunkTotalSize)
	{
		UE_LOG(LOGHITerrain, Error, TEXT("UHITerrainDataBase::GetSample Out Of Range! [%d, %d]"), X, Y);
		return 0.0f;
	}
	else if (!bIsGenerated)
	{
		UE_LOG(LOGHITerrain, Error, TEXT("UHITerrainDataBase::GetSample Not Generated!"));
		return 0.0f;
	}
	else
	{
		TPair<int32, int32> Index(X / (ChunkSampleNums + 1), Y / (ChunkSampleNums + 1));
		return ChunkData[Index]->GetSample(X % (ChunkSampleNums + 1), Y % (ChunkSampleNums + 1));
	}
}

void UHITerrainDataBase::SetSample(int32 X, int32 Y, float Value)
{
	int32 ChunkTotalSize = ChunkNums * (ChunkSampleNums + 1);
	if (X < 0 || X >= ChunkTotalSize || Y < 0 || Y >= ChunkTotalSize)
	{
		UE_LOG(LOGHITerrain, Error, TEXT("UHITerrainDataBase::SetSample Out Of Range! [%d, %d]"), X, Y);
	}
	else if (!bIsGenerated)
	{
		UE_LOG(LOGHITerrain, Error, TEXT("UHITerrainDataBase::SetSample Not Generated!"));
	}
	else
	{
		TPair<int32, int32> Index(X / (ChunkSampleNums + 1), Y / (ChunkSampleNums + 1));
		ChunkData[Index]->SetSample(X % (ChunkSampleNums + 1), Y % (ChunkSampleNums + 1), Value);
	}
}

void UHITerrainDataBase::GenerateChunkData(int32 X, int32 Y)
{
	FChunkInformationPtr Data = MakeShared<FChunkInformation, ESPMode::ThreadSafe>();
	ChunkData.Add(TPair<int32, int32>(X, Y), Data);
	Data->SampleNums = ChunkSampleNums;
	
}

void UHITerrainDataBase::SetMountainHeight(float InMountainHeight)
{
	MountainHeight = InMountainHeight;
}

void UHITerrainDataBase::SetMountainScale(float InMountainScale)
{
	MountainScale = InMountainScale;
}

void UHITerrainDataBase::SetPlainHeight(float InPlainHeight)
{
	PlainHeight = InPlainHeight;
}

void UHITerrainDataBase::SetPlainScale(float InPlainScale)
{
	PlainScale = InPlainScale;
}

void UHITerrainDataBase::SetPlainThreshold(float InPlainThreshold)
{
	PlainThreshold = InPlainThreshold;
}