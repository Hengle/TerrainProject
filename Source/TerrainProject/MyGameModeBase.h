// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "HITerrainCommons.h"
#include "MyGameModeBase.generated.h"

/**
 * 
 */
UCLASS()
class TERRAINPROJECT_API AMyGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	void CreateTerrain();

	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerrainInformation")
		int32 Seed = 10086;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerrainInformation")
		int32 ChunkNum = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerrainInformation")
		FVector Position = FVector(0, 0, 0);

private:
	UPROPERTY()
		class AHITerrainInstance* TerrainInstance;
};
