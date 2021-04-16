// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"


#include "HITerrainDataValue.h"
#include "TerrainMaths/2DArray.h"

/*
 * 内部类，纯粹是因为TFixed2DArray<TSharedPtr<FHITerrainDataValue>>太难看了所以加了一层。
 */
class FHITerrainDataChannel
{
public:
	FHITerrainDataChannel(int32 InSizeX, int32 InSizeY, ETerrainDataType InType, FString InChannelName);

	TSharedPtr<FHITerrainDataValue> GetChannelValue(int32 X, int32 Y);

	void SetChannelValue(int32 X, int32 Y, TSharedPtr<FHITerrainDataValue> Value);

	ETerrainDataType GetType();

private:
	TFixed2DArray<TSharedPtr<FHITerrainDataValue>> Data;

private:
	int32 SizeX;
	int32 SizeY;
	ETerrainDataType Type;
	FString ChannelName;
};
