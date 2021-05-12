// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TerrainDatas/HITerrainData.h"

/*
 * 数学模块类
 * 表示可以对地形数据进行改动的单一功能模块
 */
class HITERRAINMAKER_API FHITerrainModule
{
public:
	FHITerrainModule();
	FHITerrainModule(const FHITerrainModule& AnotherModule) = delete;
	void operator= (const FHITerrainModule& AnotherModule) = delete;
	virtual ~FHITerrainModule();

	/*
	 * 对地形数据应用模块
	 */
	virtual void ApplyModule(UHITerrainData* Data) = 0;
};
