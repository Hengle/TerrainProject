// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TerrainAlgorithms/HITerrainAlgorithm.h"
#include "UObject/Object.h"
#include "SampleAlgorithm.generated.h"

/**
 * 
 */
UCLASS()
class HITERRAINMAKER_API USampleAlgorithm : public UHITerrainAlgorithm
{
	GENERATED_BODY()

public:
	void SetSampleData(int32 Seed, float Frequency, float Lacunarity, float SeaLevel);
	virtual void Apply(UHITerrainData* Data) override;

private:
};
