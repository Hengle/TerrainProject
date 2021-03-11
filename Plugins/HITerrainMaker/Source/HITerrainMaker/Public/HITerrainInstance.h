#pragma once
#include "CoreMinimal.h"
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

	UMaterialInterface* Material;


private:
	void TickChunks();
	void OnDataGenerated();
	void ProcessQueue();
	bool CreateThunk(TPair<int32, int32> Index);

private:
	FTerrainInformation TerrainInformation;

	class UHITerrainDataBase* Data;

	TMap<TPair<int32, int32>, class AHITerrainActor*> Chunks;

	TQueue<TPair<int32, int32>> CreateChunkQueue;

	float ProcessQueueInterval = FLAT_CHUNK_INTERVAL;
	float ChunkSize = FLAT_CHUNK_SIZE;
	float RenderDistance = FLAT_RENDER_DISTANCE;
	FTimerHandle TimerHandle;
};