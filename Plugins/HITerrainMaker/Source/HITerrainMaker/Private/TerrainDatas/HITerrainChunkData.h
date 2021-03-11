#pragma once
#include "CoreMinimal.h"

struct FHITerrainChunkData
{
	TPair<int32, int32> Index;	// Chunk索引
	int32 ChunkSize;	// Chunk大小
	class UHITerrainData* Data;	// 采样点数据
	
	float GetSample(int32 X, int32 Y);
};

typedef TSharedPtr<FHITerrainChunkData, ESPMode::ThreadSafe> FChunkDataPtr;