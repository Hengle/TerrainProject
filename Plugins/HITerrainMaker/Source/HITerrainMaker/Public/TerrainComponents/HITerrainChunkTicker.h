// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HITerrainCommons.h"
#include "HITerrainInstance.h"

#include "HITerrainChunkTicker.generated.h"

/**
 * 
 */
UCLASS()
class HITERRAINMAKER_API UHITerrainChunkTicker : public UActorComponent
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

public:
	void TickChunks();
	
protected:
	void ProcessQueue();
	
	
protected:
	UPROPERTY()
	AHITerrainInstance* TerrainInstance;
	
	FTerrainInformationPtr TerrainInformation;
	
	TQueue<TPair<int32, int32>> CreateChunkQueue;
	float ChunkSize = FLAT_CHUNK_SIZE;
	float RenderDistance = FLAT_RENDER_DISTANCE;
	float ProcessQueueInterval = FLAT_CHUNK_INTERVAL;
	FTimerHandle TimerHandle;
};
