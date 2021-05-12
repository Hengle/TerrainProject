// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HITerrainCommons.h"
#include "HITerrainInstance.h"

#include "HITerrainChunkTicker.generated.h"

/*
 * 地形Chunk更新类 （TerrainInstance Component）
 * 负责每逻辑帧更新每个Chunk是否需要创建、更新、销毁，
 * 并且维护队列以定时创建Chunk，防止卡顿。
 */
UCLASS()
class HITERRAINMAKER_API UHITerrainChunkTicker : public UActorComponent
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

public:
	/*
	 * [主线程] 对每个区块判断是否需要创建、更新、销毁
	 * 在ATerrainInstance的Tick中调用
	 */
	void TickChunks();
	
private:
	/*
	 * [主线程] 处理生成队列与更新队列
	 * 这里是使用Timer做的异步
	 */
	void ProcessQueue();
	
private:
	int32 ProcessTime = 0;
	
	// 所属的AHITerrainInstance
	UPROPERTY()
	AHITerrainInstance* TerrainInstance;

	// 地形信息
	FTerrainInformationPtr TerrainInformation;

	// 区块生成队列
	TQueue<TPair<int32, int32>> CreateChunkQueue;

	// 区块更新队列
	TQueue<TPair<int32, int32>> UpdateChunkQueue;

	float RenderDistance;
	float ProcessQueueInterval;
	FTimerHandle TimerHandle;
};
