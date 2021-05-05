// Fill out your copyright notice in the Description page of Project Settings.


#include "HITerrainChannel.h"

TSharedPtr<FHITerrainChannel> FHITerrainChannel::CreateChannelByType(FString InChannelName, int32 InSizeX,
	int32 InSizeY, ETerrainDataType InType)
{
	if(InType == ETerrainDataType::FLOAT)
	{
		return CreateFloatChannel(InChannelName, InSizeX, InSizeY);
	}
	else if(InType == ETerrainDataType::BOOL)
	{
		return CreateBoolChannel(InChannelName, InSizeX, InSizeY);
	}
	else if(InType == ETerrainDataType::FVECTOR)
	{
		return CreateFVectorChannel(InChannelName, InSizeX, InSizeY);
	}
	else if(InType == ETerrainDataType::FQUAT)
	{
		return CreateFQuatChannel(InChannelName, InSizeX, InSizeY);
	}
	return nullptr;
}

TSharedPtr<FHITerrainChannel> FHITerrainChannel::CreateFloatChannel(FString InChannelName, int32 InSizeX, int32 InSizeY)
{
	TSharedPtr<FHITerrainChannel> Channel = MakeShared<FHITerrainFloatChannel>(InSizeX, InSizeY);
	Channel->ChannelName = InChannelName;
	Channel->SizeX = InSizeX;
	Channel->SizeY = InSizeY;
	return Channel;
}

TSharedPtr<FHITerrainChannel> FHITerrainChannel::CreateBoolChannel(FString InChannelName, int32 InSizeX, int32 InSizeY)
{
	TSharedPtr<FHITerrainChannel> Channel = MakeShared<FHITerrainBoolChannel>(InSizeX, InSizeY);
	Channel->ChannelName = InChannelName;
	Channel->SizeX = InSizeX;
	Channel->SizeY = InSizeY;
	return Channel;
}

TSharedPtr<FHITerrainChannel> FHITerrainChannel::CreateFVectorChannel(FString InChannelName, int32 InSizeX,
	int32 InSizeY)
{
	TSharedPtr<FHITerrainChannel> Channel = MakeShared<FHITerrainFVectorChannel>(InSizeX, InSizeY);
	Channel->ChannelName = InChannelName;
	Channel->SizeX = InSizeX;
	Channel->SizeY = InSizeY;
	return Channel;
}

TSharedPtr<FHITerrainChannel> FHITerrainChannel::CreateFQuatChannel(FString InChannelName, int32 InSizeX, int32 InSizeY)
{
	TSharedPtr<FHITerrainChannel> Channel = MakeShared<FHITerrainFQuatChannel>(InSizeX, InSizeY);
	Channel->ChannelName = InChannelName;
	Channel->SizeX = InSizeX;
	Channel->SizeY = InSizeY;
	return Channel;
}

TSharedPtr<FHITerrainChannel> FHITerrainChannel::CopyChannel(FString InChannelName,
	TSharedPtr<FHITerrainChannel> FromChannel)
{
	//TODO
	return nullptr;
}

float FHITerrainChannel::GetFloat(int32 X, int32 Y)
{
	float Value = 0.0f;
	if(!TryGetFloat(X, Y, Value))
	{
		ErrorMessage(TEXT("Float"));
	}
	return Value;
}

bool FHITerrainChannel::TryGetFloat(int32 X, int32 Y, float& OutFloat)
{
	return false;
}

void FHITerrainChannel::SetFloat(int32 X, int32 Y, const float& InFloat)
{
}

FVector FHITerrainChannel::GetFVector(int32 X, int32 Y)
{
	FVector Value = FVector();
	if(!TryGetFVector(X, Y, Value))
	{
		ErrorMessage(TEXT("FVector"));
	}
	return Value;
}

bool FHITerrainChannel::TryGetFVector(int32 X, int32 Y, FVector& OutFVector)
{
	return false;
}

void FHITerrainChannel::SetFVector(int32 X, int32 Y, const FVector& InFVector)
{
}

bool FHITerrainChannel::GetBool(int32 X, int32 Y)
{
	bool Value = false;
	if(!TryGetBool(X, Y, Value))
	{
		ErrorMessage(TEXT("Bool"));
	}
	return Value;
}

bool FHITerrainChannel::TryGetBool(int32 X, int32 Y, bool& OutBool)
{
	return false;
}

void FHITerrainChannel::SetBool(int32 X, int32 Y, const bool InBool)
{
}

FQuat FHITerrainChannel::GetFQuat(int32 X, int32 Y)
{
	FQuat Value = FQuat();
	if(!TryGetFQuat(X, Y, Value))
	{
		ErrorMessage(TEXT("FQuat"));
	}
	return Value;
}

bool FHITerrainChannel::TryGetFQuat(int32 X, int32 Y, FQuat& OutFQuat)
{
	return false;
}

void FHITerrainChannel::SetFQuat(int32 X, int32 Y, const FQuat& InFQuat)
{
}

int32 FHITerrainChannel::GetSizeX()
{
	return SizeX;
}

int32 FHITerrainChannel::GetSizeY()
{
	return SizeY;
}

void FHITerrainChannel::ErrorMessage(const FString& InType)
{
	FString RealType = GetTypeName();
	UE_LOG(LogHITerrain, Error, TEXT("Terrain Channel of type '%s' used as a '%s'."), *RealType, *InType);
}
