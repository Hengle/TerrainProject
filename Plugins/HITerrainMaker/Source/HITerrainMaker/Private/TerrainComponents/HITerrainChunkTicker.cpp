﻿// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainComponents/HITerrainChunkTicker.h"

#include "HITerrainManager.h"

void UHITerrainChunkTicker::BeginPlay()
{
	Super::BeginPlay();
	TerrainInstance = Cast<AHITerrainInstance>(GetOwner());
	TerrainInformation = TerrainInstance->GetTerrainInformation();
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &UHITerrainChunkTicker::ProcessQueue, ProcessQueueInterval, true, 0.0f);
}

void UHITerrainChunkTicker::TickChunks()
{
	FVector PlayerLocation = UHITerrainManager::Get()->GetPlayerLocation(GetWorld());
	FVector PlayerOffset = PlayerLocation - TerrainInstance->GetActorLocation();
	int32 xStart = FMath::Floor((PlayerOffset.X - RenderDistance) / ChunkSize);
	int32 xEnd = FMath::Floor((PlayerOffset.X + RenderDistance) / ChunkSize);
	int32 yStart = FMath::Floor((PlayerOffset.Y - RenderDistance) / ChunkSize);
	int32 yEnd = FMath::Floor((PlayerOffset.Y + RenderDistance) / ChunkSize);
	TSet<TPair<int32, int32>> UpdateSet;
	for (int32 x = xStart; x < xEnd; x++) {
		for (int32 y = yStart; y < yEnd; y++) {
			TPair<int32, int32> Index(x, y);
			UpdateSet.Add(Index);
			if (x < 0 || x >= TerrainInformation->ChunkNum || y < 0 || y >= TerrainInformation->ChunkNum)
			{
				continue;
			}
			if (TerrainInstance->IsChunkGenerated(Index)) 
			{
				// 更新逻辑需要放这里写还是放队列里我暂且蒙古
				// LOD中期后再写，很复杂反正
				// UpdateChunkQueue.Enqueue(Index);
				
			}
			else 
			{
				// UE_LOG(LogHITerrain, Log, TEXT("HITerrainInstance: Need ProceduralMesh[%d, %d]"), Index.Key, Index.Value)
				CreateChunkQueue.Enqueue(Index);
			}
		}
	}
	TerrainInstance->DeleteChunkNotInSet(UpdateSet);
}


void UHITerrainChunkTicker::ProcessQueue() 
{
	if (!CreateChunkQueue.IsEmpty()) 
	{
		TPair<int32, int32> Index;
		bool bCreated = false;
		while (!bCreated)
		{
			bool bSuccess = CreateChunkQueue.Dequeue(Index);
			if (bSuccess)
			{
				bCreated = TerrainInstance->GenerateChunkTerrain(Index);
			}
			else
			{
				break;
			}
		}
	}
	// if (!UpdateChunkQueue.IsEmpty()) 
	// {
	// 	TPair<int32, int32> Index;
	// 	bool bUpdated = false;
	// 	while (!bUpdated)
	// 	{
	// 		bool bSuccess = UpdateChunkQueue.Dequeue(Index);
	// 		if (bSuccess)
	// 		{
	// 			bUpdated = TerrainInstance->UpdateChunk(Index);
	// 		}
	// 		else
	// 		{
	// 			break;
	// 		}
	// 	}
	// }
}

