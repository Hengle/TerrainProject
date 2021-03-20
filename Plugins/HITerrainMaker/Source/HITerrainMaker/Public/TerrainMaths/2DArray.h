// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HITerrainCommons.h"

template<typename InElementType>
struct T2DArray
{
	T2DArray(int32 InSizeX, int32 InSizeY, InElementType DefaultValue)
	{
		SizeX = InSizeX;
		SizeY = InSizeY;
		RawArray.Reserve(SizeX * SizeY);
		for(int32 i = 0; i < SizeX * SizeY; i++)
		{
			RawArray.Add(DefaultValue);
		}
	}

	void GetValue(int32 X, int32 Y, InElementType& Value)
	{
		if(X < 0 || X >= SizeX || Y < 0 || Y > SizeY)
		{
			UE_LOG(LogHITerrain, Error, TEXT("T2DArray::GetValue Out Of Range! Index[%d, %d], Range[%d, %d]"), X, Y, SizeX, SizeY)
		}
		else
		{
			Value = RawArray[X * SizeX + Y];
		}
	}
	
	void SetValue(int32 X, int32 Y, InElementType Value)
	{
		if(X < 0 || X >= SizeX || Y < 0 || Y > SizeY)
		{
			UE_LOG(LogHITerrain, Error, TEXT("T2DArray::SetValue Out Of Range! Index[%d, %d], Range[%d, %d]"), X, Y, SizeX, SizeY)
		}
		else
		{
			RawArray[X * SizeX + Y] = Value;
		}
	}

	TArray<InElementType> RawArray;

private:
	int32 SizeX;
	int32 SizeY;
};