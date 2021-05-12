/*
* This is an independent project of an individual developer. Dear PVS-Studio, please check it.
* PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
*/
// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainComponents/HITerrainChunkTicker.h"

#include "HITerrainManager.h"

void UHITerrainChunkTicker::BeginPlay()
{
	Super::BeginPlay();
	TerrainInstance = Cast<AHITerrainInstance>(GetOwner());
	TerrainInformation = TerrainInstance->GetTerrainInformation();
	ProcessQueueInterval = TerrainInformation->ChunkGenerateInterval;
	RenderDistance = TerrainInformation->RenderDistance;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &UHITerrainChunkTicker::ProcessQueue, ProcessQueueInterval, true, 0.0f);
}

void UHITerrainChunkTicker::TickChunks()
{
	TPair<int32, int32> PlayerIndex = TerrainInstance->GetPlayerPositionIndex();
	int32 xStart = PlayerIndex.Key - RenderDistance;
	int32 xEnd = PlayerIndex.Key + RenderDistance;
	int32 yStart = PlayerIndex.Value - RenderDistance;
	int32 yEnd = PlayerIndex.Value + RenderDistance;
	TSet<TPair<int32, int32>> UpdateSet;
	// UE_LOG(LogHITerrain, Log, TEXT("UHITerrainChunkTicker::TickChunks: x0:%d x1:%d y0:%d y1:%d"), xStart, xEnd, yStart, yEnd);
	// 对视野范围内的Chunk进行遍历
	for (int32 x = xStart; x <= xEnd; x++)
	{
		for (int32 y = yStart; y <= yEnd; y++)
		{
			TPair<int32, int32> Index(x, y);
			UpdateSet.Add(Index);
			// Chunk在合法区域外，不管他
			if (x < 0 || x >= TerrainInformation->ChunkNum || y < 0 || y >= TerrainInformation->ChunkNum)
			{
				continue;
			}
			// Chunk在合法区域内，且已经生成
			if (TerrainInstance->IsChunkGenerated(Index)) 
			{
				UpdateChunkQueue.Enqueue(Index);
			}
			// Chunk在合法区域内，且未生成
			else 
			{
				// UE_LOG(LogHITerrain, Log, TEXT("HITerrainInstance: Need ProceduralMesh[%d, %d]"), Index.Key, Index.Value)
				CreateChunkQueue.Enqueue(Index);
			}
		}
	}
	// 删除不在视野范围内的Chunk
	TerrainInstance->DeleteChunkNotInSet(UpdateSet);
}


void UHITerrainChunkTicker::ProcessQueue() 
{
	// 生成一个区块
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
	if(ProcessTime % 10 == 0)
	{
		// 更新一个区块
		if (!UpdateChunkQueue.IsEmpty()) 
		{
			TPair<int32, int32> Index;
			bool bUpdated = false;
			while (!bUpdated)
			{
				bool bSuccess = UpdateChunkQueue.Dequeue(Index);
				if (bSuccess)
				{
					bUpdated = TerrainInstance->UpdateChunk(Index);
				}
				else
				{
					break;
				}
			}
		}
	}
	ProcessTime ++;
}

