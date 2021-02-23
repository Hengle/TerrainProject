// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TerrainUtils.generated.h"

USTRUCT(BlueprintType)
struct TERRAINPROJECT_API FTerrainActorParameter {
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ActorIndexX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ActorIndexY;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 SectionNumX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 SectionNumY;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SectionInterval;
};

USTRUCT(BlueprintType)
struct TERRAINPROJECT_API FTerrainTaskParameter {
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SizeX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SizeY;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float StepX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float StepY;
};

USTRUCT(BlueprintType)
struct TERRAINPROJECT_API FTerrainMeshSectionParameter {
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 SectionIndex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 XIndex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 YIndex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTerrainTaskParameter TaskParam;
};

class TERRAINPROJECT_API TerrainUtils
{
};
