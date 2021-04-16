/*
* This is an independent project of an individual developer. Dear PVS-Studio, please check it.
* PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
*/
#include "HITerrainData.h"

#include "TerrainMaths/HITerrainMathMisc.h"

uint32 UHITerrainData::Run()
{
	int32 TotalSize = Size();
	// TerrainData = TFixed2DArray<FTerrainSample>(TotalSize, TotalSize, FTerrainSample());
	AddChannel("height", ETerrainDataType::FLOAT);
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

float UHITerrainData::GetHeightValue(int32 X, int32 Y)
{
	if (!bIsGenerated)
	{
		UE_LOG(LogHITerrain, Error, TEXT("UHITerrainDataBase::GetSample Not Generated!"));
		return 0.0f;
	}
	else
	{
		// return TerrainData.GetValue(X, Y).Value;
		return TerrainDataChannels["height"]->GetChannelValue(X, Y)->GetNumber();
	}
}

void UHITerrainData::SetHeightValue(int32 X, int32 Y, float Value)
{
	if (!bIsGenerated)
	{
		UE_LOG(LogHITerrain, Error, TEXT("UHITerrainDataBase::SetSample Not Generated!"));
	}
	else
	{
		// TerrainData.GetValueRef(X, Y).Value = Value;
		TerrainDataChannels["height"]->SetChannelValue(X, Y, MakeShareable<FHITerrainDataValue>(new FHITerrainFloatValue(Value)));
	}
}

float UHITerrainData::GetHeightValue(float X, float Y)
{
	// TODO: 现在其实不用插值了。。。
	if (!bIsGenerated)
	{
		UE_LOG(LogHITerrain, Error, TEXT("UHITerrainDataBase::GetSample Not Generated!"));
		return 0.0f;
	}
	else
	{
		float XFloor = FMath::FloorToInt(X / 100);
		float YFloor = FMath::FloorToInt(Y / 100);
		float XCeil = FMath::CeilToInt(X / 100);
		float YCeil = FMath::CeilToInt(Y / 100);
		float Alpha0 = X / 100 - FMath::FloorToInt(X / 100);
		float Alpha1 = Y / 100 - FMath::FloorToInt(Y / 100);
		// if(Alpha0 == 0 && Alpha1 == 0)
		// {
		// 	return TerrainData.GetValue(FMath::FloorToInt(X / 100), FMath::FloorToInt(Y / 100)).Value;
		// }
		float Value00 = GetHeightValue(FMath::FloorToInt(X / 100), FMath::FloorToInt(Y / 100));
		float Value01 = GetHeightValue(FMath::FloorToInt(X / 100), FMath::CeilToInt(Y / 100));
		float Value10 = GetHeightValue(FMath::CeilToInt(X / 100), FMath::FloorToInt(Y / 100));
		float Value11 = GetHeightValue(FMath::CeilToInt(X / 100), FMath::CeilToInt(Y / 100));
		float Value = UHITerrainMathMisc::Lerp2D(Value00, Value01, Value10, Value11, Alpha0, Alpha1);
		return Value;
	}
}

void UHITerrainData::AddChannel(FString ChannelName, ETerrainDataType Type)
{
	if(TerrainDataChannels.Contains(ChannelName))
	{
		UE_LOG(LogHITerrain, Error, TEXT("UHITerrainData::AddChannel Existing ChannelName '%s'"), *ChannelName)
	}
	else
	{
		int32 TotalSize = Size();
		TerrainDataChannels.Add(ChannelName, MakeShareable<FHITerrainDataChannel>(new FHITerrainDataChannel(TotalSize, TotalSize, Type, ChannelName)));
	}
}

TSharedPtr<FHITerrainDataChannel> UHITerrainData::GetChannel(FString ChannelName)
{
	if(!TerrainDataChannels.Contains(ChannelName))
	{
		UE_LOG(LogHITerrain, Error, TEXT("UHITerrainData::GetChannel Error ChannelName '%s'"), *ChannelName)
	}
	return TerrainDataChannels[ChannelName];
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
