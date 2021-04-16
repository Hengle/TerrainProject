// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"


#include "HITerrainDataValue.h"
#include "TerrainMaths/2DArray.h"


class HITERRAINMAKER_API FHITerrainDataChannel
{
public:
	FHITerrainDataChannel(int32 InSizeX, int32 InSizeY, ETerrainDataType InType, FString InChannelName);

private:
	TFixed2DArray<TSharedPtr<FHITerrainDataValue>> Data;

private:
	int32 SizeX;
	int32 SizeY;
	ETerrainDataType Type;
	FString ChannelName;
};
