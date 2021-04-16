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
}
