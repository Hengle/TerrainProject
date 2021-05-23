// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "HITerrainModule.h"


class HITERRAINMAKER_API FHITerrainHumidityGPU : public FHITerrainModule
{
public:
	virtual void ApplyModule(UHITerrainData* Data) override;
};
