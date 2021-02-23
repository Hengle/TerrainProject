// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TerrainActor.h"
#include "TerrainUtils.h"
#include "TerrainDataManager.h"
#include "TerrainProjectGameMode.generated.h"

UCLASS(minimalapi)
class ATerrainProjectGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ATerrainProjectGameMode();

	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerrainDataManagerParameters")
	int32 Seed = 10086;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerrainDataManagerParameters")
	float LocationScale = 0.1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerrainDataManagerParameters")
	float HeightScale = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerrainActorParameters")
	int32 ActorNumX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerrainActorParameters")
	int32 ActorNumY;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerrainActorParameters")
	FTerrainActorParameter ActorParameter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerrainTaskParameters")
	FTerrainTaskParameter TaskParameter;

private:
	UPROPERTY()
	TArray<ATerrainActor*> TerrainActors;

	int32 ActorIndex = 0;

	int32 ActorRecentX = 0;

	int32 ActorRecentY = 0;

	FTimerHandle TimerHandle;

private:
	void CreateTerrainActor();
	
	void InitDataManager();
};



