#pragma once
#include "CoreMinimal.h"
#include "HITerrainCommons.h"
#include "TerrainAlgorithms/HITerrainAlgorithm.h"
#include "HITerrainInstance.generated.h"

/*
 * 地形实例
 */
UCLASS()
class HITERRAINMAKER_API AHITerrainInstance :public AActor
{
	GENERATED_BODY()

public:
	FTerrainInformationPtr GetTerrainInformation() const;

	bool ContainsChunk(TPair<int32, int32> Index) const;

	void AddChunk(TPair<int32, int32> Index);

	bool GenerateChunkTerrain(TPair<int32, int32> Index);

	

public:
	AHITerrainInstance();

	void Init(FTerrainInformationPtr InTerrainInformation);

	virtual void Tick(float DeltaTime) override;

	UPROPERTY()
	UMaterialInterface* Material;


private:
	void InitAlgorithms();
	
	void OnDataGenerated();

private:
	UPROPERTY()
	class UHITerrainChunkTicker* ChunkTicker;
	
	FTerrainInformationPtr TerrainInformation;

	UPROPERTY()
	TArray<UHITerrainAlgorithm*> Algorithms;

	UPROPERTY()
	class UHITerrainData* Data;

	TMap<TPair<int32, int32>, class AHITerrainActor*> Chunks;
};