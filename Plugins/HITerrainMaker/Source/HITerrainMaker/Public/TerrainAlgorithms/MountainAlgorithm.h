// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "HITerrainAlgorithm.h"
#include "TerrainDatas/HITerrainPerlinGenerator.h"
#include "UObject/Object.h"
#include "MountainAlgorithm.generated.h"

/**
 * 
 */
UCLASS()
class HITERRAINMAKER_API UMountainAlgorithm : public UHITerrainAlgorithm
{
	GENERATED_BODY()

public:
	void SetMountainData(int32 InSeed, float InMountainHeight, float InMountainScale);

	virtual void Apply(UHITerrainData* Data) override;

private:
	float MountainHeight;
	float MountainScale;
	HITerrainPerlinGenerator MountainGenerator;
};
