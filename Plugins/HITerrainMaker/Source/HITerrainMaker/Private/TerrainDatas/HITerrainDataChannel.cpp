// Fill out your copyright notice in the Description page of Project Settings.


#include "HITerrainDataChannel.h"

FHITerrainDataChannel::FHITerrainDataChannel(int32 InSizeX, int32 InSizeY, ETerrainDataType InType,
	FString InChannelName):SizeX(InSizeX), SizeY(InSizeY), Type(InType), ChannelName(InChannelName)
{
	Data = TFixed2DArray<TSharedPtr<FHITerrainDataValue>>(SizeX, SizeY, nullptr);
	if(Type == ETerrainDataType::FLOAT)
	{
		for(int32 i = 0; i < SizeX; i++)
		{
			for(int32 j = 0; j < SizeY; j++)
			{
				Data.SetValue(i, j, MakeShared<FHITerrainFloatValue>(0.0f));
			}
		}
	}
	else if(Type == ETerrainDataType::FVECTOR)
	{
		for(int32 i = 0; i < SizeX; i++)
		{
			for(int32 j = 0; j < SizeY; j++)
			{
				Data.SetValue(i, j, MakeShared<FHITerrainFVectorValue>(FVector()));
			}
		}
	}
	else if(Type == ETerrainDataType::BOOL)
	{
		for(int32 i = 0; i < SizeX; i++)
		{
			for(int32 j = 0; j < SizeY; j++)
			{
				Data.SetValue(i, j, MakeShared<FHITerrainBoolValue>(false));
			}
		}
	}
	else if(Type == ETerrainDataType::FQUAT)
	{
		for(int32 i = 0; i < SizeX; i++)
		{
			for(int32 j = 0; j < SizeY; j++)
			{
				Data.SetValue(i, j, MakeShared<FHITerrainFQuatValue>(FQuat()));
			}
		}
	}
}

FHITerrainDataChannel::FHITerrainDataChannel(TSharedPtr<FHITerrainDataChannel> FromChannel)
{
	SizeX = FromChannel->GetSizeX();
	SizeY = FromChannel->GetSizeY();
	Type = FromChannel->GetType();
	Data = TFixed2DArray<TSharedPtr<FHITerrainDataValue>>(SizeX, SizeY, nullptr);
	if(Type == ETerrainDataType::FLOAT)
	{
		for(int32 i = 0; i < SizeX; i++)
		{
			for(int32 j = 0; j < SizeY; j++)
			{
				Data.SetValue(i, j, MakeShared<FHITerrainFloatValue>(FromChannel->GetValue(i, j)->GetNumber()));
			}
		}
	}
	else if(Type == ETerrainDataType::FVECTOR)
	{
		for(int32 i = 0; i < SizeX; i++)
		{
			for(int32 j = 0; j < SizeY; j++)
			{
				Data.SetValue(i, j, MakeShared<FHITerrainFVectorValue>(FromChannel->GetValue(i, j)->GetFVector()));
			}
		}
	}
	else if(Type == ETerrainDataType::BOOL)
	{
		for(int32 i = 0; i < SizeX; i++)
		{
			for(int32 j = 0; j < SizeY; j++)
			{
				Data.SetValue(i, j, MakeShared<FHITerrainBoolValue>(FromChannel->GetValue(i, j)->GetBool()));
			}
		}
	}
	else if(Type == ETerrainDataType::FQUAT)
	{
		for(int32 i = 0; i < SizeX; i++)
		{
			for(int32 j = 0; j < SizeY; j++)
			{
				Data.SetValue(i, j, MakeShared<FHITerrainFQuatValue>(FromChannel->GetValue(i, j)->GetFQuat()));
			}
		}
	}
}

TSharedPtr<FHITerrainDataValue> FHITerrainDataChannel::GetValue(int32 X, int32 Y)
{
	return Data.GetValue(X, Y);	
}

void FHITerrainDataChannel::SetValue(int32 X, int32 Y, TSharedPtr<FHITerrainDataValue> Value)
{
	Data.SetValue(X, Y, Value);
}

void FHITerrainDataChannel::CopyFromChannel(TSharedPtr<FHITerrainDataChannel> FromChannel)
{
	for(int32 i = 0; i < SizeX; i++)
	{
		for(int32 j = 0; j < SizeY; j++)
		{
			Data.GetValue(i, j)->CopyFromValue(FromChannel->GetValue(i, j));
		}
	}
}

void FHITerrainDataChannel::SetValue(int32 X, int32 Y, const float& Value)
{
	Data.GetValueRef(X, Y)->SetNumber(Value);
}

void FHITerrainDataChannel::SetValue(int32 X, int32 Y, const FVector& Value)
{
	Data.GetValueRef(X, Y)->SetFVector(Value);
}

void FHITerrainDataChannel::SetValue(int32 X, int32 Y, const bool Value)
{
	Data.GetValueRef(X, Y)->SetBool(Value);
}

void FHITerrainDataChannel::SetValue(int32 X, int32 Y, const FQuat& Value)
{
	Data.GetValueRef(X, Y)->SetFQuat(Value);
}

ETerrainDataType FHITerrainDataChannel::GetType()
{
	return Type;
}

int32 FHITerrainDataChannel::GetSizeX()
{
	return SizeX;
}

int32 FHITerrainDataChannel::GetSizeY()
{
	return SizeY;
}
