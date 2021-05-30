#include "HITerrainData.h"

#include "Kismet/KismetMathLibrary.h"
#include "TerrainMaths/HITerrainMathMisc.h"

uint32 UHITerrainData::Run()
{
	TotalSize = ChunkSize * ChunkNums + 1;
	RealTotalSize = Information->RealTotalSize;
	AddChannel("height", ETerrainDataType::FLOAT);
	bIsGenerated = true;

	for(int32 i = 0; i < ChunkNums; i++)
	{
		for(int32 j = 0; j < ChunkNums; j++)
		{
			TPair<int32, int32> Index(i, j);
			// GrassData.Add(Index, TArray<FVector>());
			FoliageData.Add(Index, TArray<FFoliageData>());
		}
	}
	
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

int32 UHITerrainData::GetIndex(int32 X, int32 Y, int32 InTotalSize)
{
	return X * InTotalSize + Y;
}

void UHITerrainData::ApplyAlgorithm(UHITerrainAlgorithm* Algorithm)
{

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
		return TerrainDataChannels["height"]->GetFloat(X, Y);
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
		TerrainDataChannels["height"]->SetFloat(X, Y, Value);
	}
}

float UHITerrainData::GetHeightValue(float X, float Y)
{
	// 现在其实不用插值了，但这样也没有什么大问题
	if (!bIsGenerated)
	{
		UE_LOG(LogHITerrain, Error, TEXT("UHITerrainDataBase::GetSample Not Generated!"));
		return 0.0f;
	}
	else
	{
		int32 XFloor = FMath::FloorToInt(X / 100);
		int32 YFloor = FMath::FloorToInt(Y / 100);
		int32 XCeil = FMath::CeilToInt(X / 100);
		int32 YCeil = FMath::CeilToInt(Y / 100);
		float Alpha0 = X / 100 - XFloor;
		float Alpha1 = Y / 100 - YFloor;
		float Value00 = GetHeightValue(XFloor, YFloor);
		float Value01 = GetHeightValue(XFloor, YCeil);
		float Value10 = GetHeightValue(XCeil, YFloor);
		float Value11 = GetHeightValue(XCeil, YCeil);
		float Value = FHITerrainMathMisc::LinearLerp2D(Value00, Value01, Value10, Value11, Alpha0, Alpha1);
		return Value;
	}
}

float UHITerrainData::GetSedimentValue(float X, float Y)
{
	if (!bIsGenerated)
	{
		UE_LOG(LogHITerrain, Error, TEXT("UHITerrainDataBase::GetSample Not Generated!"));
		return 0.0f;
	}
	else
	{
		int32 XFloor = FMath::FloorToInt(X / 100);
		int32 YFloor = FMath::FloorToInt(Y / 100);
		int32 XCeil = FMath::CeilToInt(X / 100);
		int32 YCeil = FMath::CeilToInt(Y / 100);
		float Alpha0 = X / 100 - XFloor;
		float Alpha1 = Y / 100 - YFloor;
		float Value00;
		float Value01;
		float Value10;
		float Value11;
		GetChannelValue("sediment", XFloor, YFloor, Value00);
		GetChannelValue("sediment", XFloor, YCeil, Value01);
		GetChannelValue("sediment", XCeil, YFloor, Value10);
		GetChannelValue("sediment", XCeil, YCeil, Value11);
		
		float Value = FHITerrainMathMisc::LinearLerp2D(Value00, Value01, Value10, Value11, Alpha0, Alpha1);
		return Value;
	}
}

float UHITerrainData::GetSedimentValue(int32 X, int32 Y)
{
	if (!bIsGenerated)
	{
		UE_LOG(LogHITerrain, Error, TEXT("UHITerrainDataBase::GetSample Not Generated!"));
		return 0.0f;
	}
	else
	{
		return TerrainDataChannels["sediment"]->GetFloat(X, Y);
	}
}

float UHITerrainData::GetWaterValue(float X, float Y)
{
	if (!bIsGenerated)
	{
		UE_LOG(LogHITerrain, Error, TEXT("UHITerrainDataBase::GetSample Not Generated!"));
		return 0.0f;
	}
	else
	{
		int32 XFloor = FMath::FloorToInt(X / 100);
		int32 YFloor = FMath::FloorToInt(Y / 100);
		int32 XCeil = FMath::CeilToInt(X / 100);
		int32 YCeil = FMath::CeilToInt(Y / 100);
		float Alpha0 = X / 100 - XFloor;
		float Alpha1 = Y / 100 - YFloor;
		float Value00;
		float Value01;
		float Value10;
		float Value11;
		GetChannelValue("water", XFloor, YFloor, Value00);
		GetChannelValue("water", XFloor, YCeil, Value01);
		GetChannelValue("water", XCeil, YFloor, Value10);
		GetChannelValue("water", XCeil, YCeil, Value11);
		
		float Value = FHITerrainMathMisc::LinearLerp2D(Value00, Value01, Value10, Value11, Alpha0, Alpha1);
		return Value;
	}
}

float UHITerrainData::GetWaterValue(int32 X, int32 Y)
{
	if (!bIsGenerated)
	{
		UE_LOG(LogHITerrain, Error, TEXT("UHITerrainDataBase::GetSample Not Generated!"));
		return 0.0f;
	}
	else
	{
		return TerrainDataChannels["water"]->GetFloat(X, Y);
	}
}

void UHITerrainData::AddChannel(FString ChannelName, ETerrainDataType Type)
{
	if(!TerrainDataChannels.Contains(ChannelName))
	{
		TerrainDataChannels.Add(ChannelName, FHITerrainChannel::CreateChannelByType(ChannelName, RealTotalSize, RealTotalSize, Type));
	}
	else if(TerrainDataChannels[ChannelName]->GetType() != Type)
	{
		UE_LOG(LogHITerrain, Error, TEXT("UHITerrainData::AddChannel Existing Channel With Different Type!"))
	}
}

void UHITerrainData::AddChannel(FString ChannelName, TSharedPtr<FHITerrainChannel> FromChannel)
{
	if(!TerrainDataChannels.Contains(ChannelName))
	{
		//TODO
		TerrainDataChannels.Add(ChannelName, FHITerrainChannel::CopyChannel(ChannelName, FromChannel));
	}
}

void UHITerrainData::DeleteChannel(FString ChannelName)
{
	if(!TerrainDataChannels.Contains(ChannelName))
	{
		UE_LOG(LogHITerrain, Error, TEXT("UHITerrainData::DeleteChannel No ChannelName '%s'"), *ChannelName)
	}
	else
	{
		TerrainDataChannels.Remove(ChannelName);
	}
}

void UHITerrainData::CopyChannel(FString FromChannelName, FString ToChannelName)
{
	if(!TerrainDataChannels.Contains(FromChannelName))
	{
		UE_LOG(LogHITerrain, Error, TEXT("UHITerrainData::CopyChannel No ChannelName '%s'"), *FromChannelName)
	}
	else
	{
		if(TerrainDataChannels.Contains(ToChannelName))
		{
			if(TerrainDataChannels[ToChannelName]->GetTypeName() != TerrainDataChannels[FromChannelName]->GetTypeName())
			{
				UE_LOG(LogHITerrain, Error, TEXT("UHITerrainData::CopyChannel Type Error"))
			}
			else
			{

			}
		}
		else
		{
			AddChannel(ToChannelName, TerrainDataChannels[FromChannelName]);
		}
	}
}

TSharedPtr<FHITerrainChannel> UHITerrainData::GetChannel(FString ChannelName)
{
	if(!TerrainDataChannels.Contains(ChannelName))
	{
		UE_LOG(LogHITerrain, Error, TEXT("UHITerrainData::GetChannel Error ChannelName '%s'"), *ChannelName)
	}
	return TerrainDataChannels[ChannelName];
}

bool UHITerrainData::ContainsChannel(FString ChannelName)
{
	return TerrainDataChannels.Contains(ChannelName);
}

bool UHITerrainData::GetChannelValue(FString ChannelName, int32 X, int32 Y, float& Value)
{
	if(!TerrainDataChannels.Contains(ChannelName))
	{
		UE_LOG(LogHITerrain, Error, TEXT("UHITerrainData::GetChannelValue Error ChannelName '%s'"), *ChannelName)
		return false;
	}
	else
	{
		if(TerrainDataChannels[ChannelName]->TryGetFloat(X, Y, Value))
		{
			return true;		
		}
		else
		{
			return false;
		}
	}
}

bool UHITerrainData::GetChannelValue(FString ChannelName, float X, float Y, float& Value)
{
	if (!bIsGenerated)
	{
		UE_LOG(LogHITerrain, Error, TEXT("UHITerrainDataBase::GetSample Not Generated!"));
		return false;
	}
	else
	{
		int32 XFloor = FMath::FloorToInt(X / 100);
		int32 YFloor = FMath::FloorToInt(Y / 100);
		int32 XCeil = FMath::CeilToInt(X / 100);
		int32 YCeil = FMath::CeilToInt(Y / 100);
		float Alpha0 = X / 100 - XFloor;
		float Alpha1 = Y / 100 - YFloor;
		float Value00;
		float Value01;
		float Value10;
		float Value11;
		GetChannelValue(ChannelName, XFloor, YFloor, Value00);
		GetChannelValue(ChannelName, XFloor, YCeil, Value01);
		GetChannelValue(ChannelName, XCeil, YFloor, Value10);
		GetChannelValue(ChannelName, XCeil, YCeil, Value11);
		
		Value = FHITerrainMathMisc::LinearLerp2D(Value00, Value01, Value10, Value11, Alpha0, Alpha1);
		return true;
	}
}

bool UHITerrainData::SetChannelValue(FString ChannelName, int32 X, int32 Y, const float& Value)
{
	if(!TerrainDataChannels.Contains(ChannelName))
	{
		UE_LOG(LogHITerrain, Error, TEXT("UHITerrainData::SetChannelValue Error ChannelName '%s'"), *ChannelName)
		return false;
	}
	else
	{
		if(TerrainDataChannels[ChannelName]->GetType() == ETerrainDataType::FLOAT)
		{
			TerrainDataChannels[ChannelName]->SetFloat(X, Y, Value);
			return true;		
		}
		else
		{
			return false;
		}
	}
}

bool UHITerrainData::GetChannelValue(FString ChannelName, int32 X, int32 Y, FVector& Value)
{
	if(!TerrainDataChannels.Contains(ChannelName))
	{
		UE_LOG(LogHITerrain, Error, TEXT("UHITerrainData::GetChannelValue Error ChannelName '%s'"), *ChannelName)
		return false;
	}
	else
	{
		if(TerrainDataChannels[ChannelName]->TryGetFVector(X, Y, Value))
		{
			return true;		
		}
		else
		{
			return false;
		}
	}
}

bool UHITerrainData::SetChannelValue(FString ChannelName, int32 X, int32 Y, const FVector& Value)
{
	if(!TerrainDataChannels.Contains(ChannelName))
	{
		UE_LOG(LogHITerrain, Error, TEXT("UHITerrainData::SetChannelValue Error ChannelName '%s'"), *ChannelName)
		return false;
	}
	else
	{
		if(TerrainDataChannels[ChannelName]->GetType() == ETerrainDataType::FVECTOR)
		{
			TerrainDataChannels[ChannelName]->SetFVector(X, Y, Value);
			return true;		
		}
		else
		{
			return false;
		}
	}
}

bool UHITerrainData::GetChannelValue(FString ChannelName, int32 X, int32 Y, bool& Value)
{
	if(!TerrainDataChannels.Contains(ChannelName))
	{
		UE_LOG(LogHITerrain, Error, TEXT("UHITerrainData::GetChannelValue Error ChannelName '%s'"), *ChannelName)
		return false;
	}
	else
	{
		if(TerrainDataChannels[ChannelName]->TryGetBool(X, Y, Value))
		{
			return true;		
		}
		else
		{
			return false;
		}
	}
}

bool UHITerrainData::SetChannelValue(FString ChannelName, int32 X, int32 Y, bool Value)
{
	if(!TerrainDataChannels.Contains(ChannelName))
	{
		UE_LOG(LogHITerrain, Error, TEXT("UHITerrainData::SetChannelValue Error ChannelName '%s'"), *ChannelName)
		return false;
	}
	else
	{
		if(TerrainDataChannels[ChannelName]->GetType() == ETerrainDataType::BOOL)
		{
			TerrainDataChannels[ChannelName]->SetBool(X, Y, Value);
			return true;		
		}
		else
		{
			return false;
		}
	}
}

bool UHITerrainData::GetChannelValue(FString ChannelName, int32 X, int32 Y, FQuat& Value)
{
	if(!TerrainDataChannels.Contains(ChannelName))
	{
		UE_LOG(LogHITerrain, Error, TEXT("UHITerrainData::GetChannelValue Error ChannelName '%s'"), *ChannelName)
		return false;
	}
	else
	{
		if(TerrainDataChannels[ChannelName]->TryGetFQuat(X, Y, Value))
		{
			return true;		
		}
		else
		{
			return false;
		}
	}
}

bool UHITerrainData::SetChannelValue(FString ChannelName, int32 X, int32 Y, const FQuat& Value)
{
	if(!TerrainDataChannels.Contains(ChannelName))
	{
		UE_LOG(LogHITerrain, Error, TEXT("UHITerrainData::SetChannelValue Error ChannelName '%s'"), *ChannelName)
		return false;
	}
	else
	{
		if(TerrainDataChannels[ChannelName]->GetType() == ETerrainDataType::FQUAT)
		{
			TerrainDataChannels[ChannelName]->SetFQuat(X, Y, Value);
			return true;		
		}
		else
		{
			return false;
		}
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
	return TotalSize;
}

int32 UHITerrainData::RealSize()
{
	return RealTotalSize;
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

void UHITerrainData::AddChunkGrass(TPair<int32, int32>& Index, FVector& GrassPosition)
{
	GrassData[Index].Add(GrassPosition);
}

void UHITerrainData::AddChunkFoliage(TPair<int32, int32>& Index, FFoliageData& Data)
{
	FoliageData[Index].Add(Data);
}

FRotator UHITerrainData::GetRotatorAtLocation(const FVector& Location)
{
	float Delta = 50.0f;
	FVector LeftPoint(Location.X - Delta, Location.Y - Delta, 0.0f);
	FVector RightPoint(Location.X + Delta, Location.Y + Delta, 0.0f);
	FVector TopPoint(Location.X + Delta, Location.Y - Delta, 0.0f);
	FVector BottomPoint(Location.X - Delta, Location.Y + Delta, 0.0f);
	LeftPoint.Z = GetHeightValue(LeftPoint.X, LeftPoint.Y) + GetSedimentValue(LeftPoint.X, LeftPoint.Y);
	RightPoint.Z = GetHeightValue(RightPoint.X, RightPoint.Y) + GetSedimentValue(RightPoint.X, RightPoint.Y);
	TopPoint.Z = GetHeightValue(TopPoint.X, TopPoint.Y) + GetSedimentValue(TopPoint.X, TopPoint.Y);
	BottomPoint.Z = GetHeightValue(BottomPoint.X, BottomPoint.Y) + GetSedimentValue(BottomPoint.X, BottomPoint.Y);
	FVector XVector = (LeftPoint - RightPoint);
	FVector YVector = (TopPoint - BottomPoint);
	FVector Normal = XVector * YVector;
	Normal.Normalize(10000.0f);
	FRotator Rotator = UKismetMathLibrary::MakeRotFromX(Normal);
	return Rotator;
}

TArray<FVector>& UHITerrainData::GetChunkGrass(TPair<int32, int32>& Index)
{
	return GrassData[Index];
}

TArray<FFoliageData>& UHITerrainData::GetChunkFoliage(TPair<int32, int32>& Index)
{
	return FoliageData[Index];
}
