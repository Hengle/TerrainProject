// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HITerrainCommons.h"
#include "TerrainDatas/HITerrainDataBase.h"
#include "UObject/Object.h"
#include "HITerrainAlgorithm.generated.h"

/**
 * 
 */
UCLASS()
class HITERRAINMAKER_API UHITerrainAlgorithm : public UObject
{
	GENERATED_BODY()

public:
	virtual void Apply(UHITerrainDataBase* Data);

private:
	bool bIsInited = false;
};
