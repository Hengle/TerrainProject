#pragma once
#include "CoreMinimal.h"

#include "HITerrainCommons.h"

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
	float GetHeightValue(int32 X, int32 Y);
	float GetHeightValue(float X, float Y);

	float GetChannelFloatValue(FString ChannelName, int32 X, int32 Y);
	float GetChannelFloatValue(FString ChannelName, float X, float Y);

	FVector2D GetUV(float X, float Y, float Tile);

	float GetChunkSize();

	int32 GetInnerPointSize(const ELODLevel& LODLevel);
	int32 GetMediumPointSize(const ELODLevel& LODLevel);
	int32 GetOuterPointSize(const ELODLevel& LODLevel);
	int32 GetOuterPointScale(const ELODLevel& LODLevel);
	int32 GetPointSize(const ELODLevel& LODLevel);
	
	float GetStepOfLODLevel(const ELODLevel& LODLevel);

	TArray<FVector>& GetChunkGrass();

	FRotator GetRotatorAtLocation(const FVector& Location);
};

typedef TSharedPtr<FHITerrainChunkData, ESPMode::ThreadSafe> FChunkDataPtr;