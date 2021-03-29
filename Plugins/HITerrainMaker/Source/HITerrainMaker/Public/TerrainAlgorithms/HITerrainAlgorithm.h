// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HITerrainCommons.h"
#include "UObject/Object.h"
#include "HITerrainAlgorithm.generated.h"

/*
 * 地形算法类
 */
UCLASS()
class HITERRAINMAKER_API UHITerrainAlgorithm : public UObject
{
	GENERATED_BODY()

public:
	virtual void Init(FTerrainInformationPtr InInformation);

	virtual void Apply(class UHITerrainData* Data);

	virtual void DebugApply(class UHITerrainData* Data);

protected:
	FTerrainInformationPtr Information;

	UPROPERTY()
	TArray<UHITerrainAlgorithm*> SubAlgorithms;
	
	bool bIsInited = false;
};
