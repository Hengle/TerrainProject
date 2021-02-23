#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine.h"
#include "ProceduralMeshComponent.h"
#include "HITerrainCommons.h"
#include "HITerrainInstance.generated.h"

UCLASS()
class HITERRAINMAKER_API AHITerrainInstance :public AActor
{
	GENERATED_BODY()

public:
	AHITerrainInstance();

	void Init(const FTerrainInformation& InTerrainInformation);

	virtual void Tick(float DeltaTime) override;


private:
	void TickChunks();
	void OnDataGenerated();
	void ProcessQueue();
	bool CreateThunk(TPair<int32, int32> Index);

private:
	FTerrainInformation TerrainInformation;
	UPROPERTY()
	class UHITerrainData* Data;

	TMap<TPair<int32, int32>, UProceduralMeshComponent*> Chunks;

	TQueue<TPair<int32, int32>> CreateChunkQueue;

	float ProcessQueueInterval = FLAT_CHUNK_INTERVAL;
	float RenderDistance = FLAT_RENDER_DISTANCE;
	int32 RenderChunkNum = FLAT_RENDER_CHUNKNUM;
	float ChunkSize = FLAT_CHUNK_SIZE;
	FTimerHandle TimerHandle;
};