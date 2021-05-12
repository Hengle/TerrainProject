// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HITerrainCommons.h"

/*
 * 定长2D数组结构。
 * 实际上是对一个一维数组的封装，只是存取值的时候进行一个转换。
 * 因此需要在初始化的时候设定二维的长宽和默认值，之后也无法改变数组的长宽。
 */
template<typename InElementType>
struct TFixed2DArray
{
	/*
	 * 默认的构造函数，没啥用，因此删掉
	 */
	// TFixed2DArray(){};

	/*
	 * 构造函数，指定长宽和默认值，初始化的时候所有位置都以默认值填充。
	 */
	TFixed2DArray(int32 InSizeX, int32 InSizeY, InElementType DefaultValue)
	{
		SizeX = InSizeX;
		SizeY = InSizeY;
		RawArray.Reserve(SizeX * SizeY);
		for(int32 i = 0; i < SizeX * SizeY; i++)
		{
			RawArray.Add(DefaultValue);
		}
	}

	/*
	 * 拷贝构造函数，简单地拷贝另一个TFixed2DArray的值
	 */
	TFixed2DArray(const TFixed2DArray& Another2DArray)
	{
		SizeX = Another2DArray.SizeX;
		SizeY = Another2DArray.SizeY;
		RawArray.Reserve(SizeX * SizeY);
		for(int32 i = 0; i < SizeX; i++)
		{
			for(int32 j = 0; j < SizeY; j++)
			{
				RawArray.Add(Another2DArray.GetValue(i, j));
			}
		}
	}

	/*
	 * 赋值运算符，也是简单地拷贝每一个值
	 */
	void operator= (const TFixed2DArray& Another2DArray)
	{
		SizeX = Another2DArray.SizeX;
		SizeY = Another2DArray.SizeY;
		RawArray.Reserve(SizeX * SizeY);
		for(int32 i = 0; i < SizeX; i++)
		{
			for(int32 j = 0; j < SizeY; j++)
			{
				RawArray.Add(Another2DArray.GetValue(i, j));
			}
		}
	}

	/*
	 * 获取某位置的值，越界了直接Fatal。
	 */
	const InElementType& GetValue(int32 X, int32 Y) const
	{
		if(X < 0 || X >= SizeX || Y < 0 || Y > SizeY)
		{
			UE_LOG(LogHITerrain, Fatal, TEXT("T2DArray::GetValue Out Of Range! Index[%d, %d], Range[%d, %d]"), X, Y, SizeX, SizeY)
		}
		return RawArray[X * SizeX + Y];
	}

	/*
	* 获取某位置的引用，越界了直接Fatal。
	*/
	InElementType& GetValueRef(int32 X, int32 Y)
	{
		if(X < 0 || X >= SizeX || Y < 0 || Y > SizeY)
		{
			UE_LOG(LogHITerrain, Fatal, TEXT("T2DArray::GetValue Out Of Range! Index[%d, %d], Range[%d, %d]"), X, Y, SizeX, SizeY)
		}
		return RawArray[X * SizeX + Y];
	}

	/*
	 * 设置某位置的值，越界了直接Fatal。
	 */
	void SetValue(int32 X, int32 Y, const InElementType& Value)
	{
		if(X < 0 || X >= SizeX || Y < 0 || Y > SizeY)
		{
			UE_LOG(LogHITerrain, Fatal, TEXT("T2DArray::SetValue Out Of Range! Index[%d, %d], Range[%d, %d]"), X, Y, SizeX, SizeY)
		}
		RawArray[X * SizeX + Y] = Value;
	}

	// 封装的1维数组
	TArray<InElementType> RawArray;

private:
	int32 SizeX;
	int32 SizeY;
};