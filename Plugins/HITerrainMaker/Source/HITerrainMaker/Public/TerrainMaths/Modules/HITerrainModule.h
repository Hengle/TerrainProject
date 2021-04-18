// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TerrainDatas/HITerrainData.h"

/**
 * 
 */
class HITERRAINMAKER_API FHITerrainModule
{
public:
	FHITerrainModule();
	virtual ~FHITerrainModule();

	virtual void ApplyModule(UHITerrainData* Data);
};
