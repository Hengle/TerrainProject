#pragma once
#include "CoreMinimal.h"

/*
 * UHITerrainData类的单个区块数据接口
 * 实际不保存数据，只从UHITerrainData类中取数据。
 */
struct FHITerrainChunkData
{
	TPair<int32, int32> Index;	// Chunk索引
	int32 ChunkSize;	// Chunk大小
	class UHITerrainData* Data;	// 采样点数据

	/*
	 * 获取该区块的某个采样点数据
	 */
	float GetSample(int32 X, int32 Y);
};

typedef TSharedPtr<FHITerrainChunkData, ESPMode::ThreadSafe> FChunkDataPtr;