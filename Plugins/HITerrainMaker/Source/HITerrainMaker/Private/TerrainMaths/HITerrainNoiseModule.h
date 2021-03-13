// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HITerrainCommons.h"
#include "noiselib/noise.h"

/**
 * 
 */
class HITerrainNoiseModule
{
public:
	HITerrainNoiseModule();
	virtual ~HITerrainNoiseModule() = 0;
	
public:
	virtual void Init() = 0;
	virtual float GetValue(int32 X, int32 Y) = 0;

protected:
	bool bInited = false;
};
