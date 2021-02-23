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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerrainInformation")
		float PositionScale = 0.01;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerrainInformation")
		ETerrainType TerrainType = ETerrainType::Flat_Earth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerrainInformation")
		ENoiseType TerrainNoiseType = ENoiseType::Perlin;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters")
		float HeightScale = 10000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters")
		UMaterialInterface* Material;

private:
	UPROPERTY()
		class AHITerrainInstance* TerrainInstance;
};
