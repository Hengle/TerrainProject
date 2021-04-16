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
	/*
	 * 获取TerrainInformation
	 */
	FTerrainInformationPtr GetTerrainInformation() const;

	/*
	 * 是否有特定索引的区块
	 */
	bool ContainsChunk(TPair<int32, int32> Index) const;

	/*
	 * 特定索引的区块是否已经被生成
	 */
	bool IsChunkGenerated(TPair<int32, int32> Index);

	/*
	 * 添加区块
	 */
	void AddChunk(TPair<int32, int32> Index);

	/*
	 * 删除集合外的区块
	 */
	void DeleteChunkNotInSet(const TSet<TPair<int32, int32>>& DeleteSet);

	/*
	 * 生成区块的地形
	 */
	bool GenerateChunkTerrain(TPair<int32, int32> Index);

	/*
	 * 更新区块
	 */
	bool UpdateChunk(TPair<int32, int32> Index);

	/*
	 * 获取角色的区块位置
	 */
	TPair<int32, int32> GetPlayerPositionIndex();

	/*
	 * 获取区块的LODLevel
	 */
	ELODLevel GetLODLevel(TPair<int32, int32> Index);

public:
	AHITerrainInstance();

	/*
	* 初始化TerrainInstance
	*/
	void Init(FTerrainInformationPtr InTerrainInformation);
	
	virtual void Tick(float DeltaTime) override;

public:
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