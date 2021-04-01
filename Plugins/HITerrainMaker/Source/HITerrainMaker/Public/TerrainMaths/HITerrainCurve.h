// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "HITerrainModule.h"

class HITERRAINMAKER_API FHITerrainCurve: public FHITerrainModule
{
public:
	void AddControlPoint(float InputValue, float OutputValue);

public:
	virtual float GetValue(float X, float Y) override;
	virtual int32 GetSourceModuleCapacity() override;
};


