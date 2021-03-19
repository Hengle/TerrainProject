#pragma once
#include "CoreMinimal.h"
#include "HITerrainCommons.h"
#include "TerrainAlgorithms/HITerrainAlgorithm.h"
#include "HITerrainInstance.generated.h"

/*
 * 地形实例类
 */
UCLASS()
class HITERRAINMAKER_API AHITerrainInstance :public AActor
{
	GENERATED_BODY()

public:
	void Init(FTerrainInformationPtr InTerrainInformation);
	
	FTerrainInformationPtr GetTerrainInformation() const;

	bool ContainsChunk(TPair<int32, int32> Index) const;

	bool IsChunkGenerated(TPair<int32, int32> Index);

	void AddChunk(TPair<int32, int32> Index);

	void DeleteChunkNotInSet(const TSet<TPair<int32, int32>>& DeleteSet);

	bool GenerateChunkTerrain(TPair<int32, int32> Index);

	bool UpdateChunk(TPair<int32, int32> Index);

	TPair<int32, int32> GetPlayerPositionIndex();

public:
	AHITerrainInstance();
	
	virtual void Tick(float DeltaTime) override;

	UPROPERTY()
	UMaterialInterface* Material;

private:
	void InitAlgorithms();
	
	void OnDataGenerated();

private:
	FTerrainInformationPtr TerrainInformation;

	TMap<TPair<int32, int32>, class AHITerrainActor*> Chunks;
	
	UPROPERTY()
	class UHITerrainChunkTicker* ChunkTicker;
	
	UPROPERTY()
	TArray<UHITerrainAlgorithm*> Algorithms;

	UPROPERTY()
	class UHITerrainData* Data;

	
};