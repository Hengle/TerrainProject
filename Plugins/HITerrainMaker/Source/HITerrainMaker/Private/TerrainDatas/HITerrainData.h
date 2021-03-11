#pragma once
#include "CoreMinimal.h"
#include "HITerrainCommons.h"
#include "HITerrainChunkData.h"
#include "TerrainAlgorithms/HITerrainAlgorithm.h"

#include "HITerrainData.generated.h"

DECLARE_DELEGATE(OnDataGeneratedEvent)

UCLASS()
class UHITerrainData: public UObject, public FRunnable
{
	GENERATED_BODY()

public:
	virtual void InitData(){};
	virtual uint32 Run();
	
	virtual FChunkDataPtr GetChunkData(const TPair<int32, int32>& Index);
	float GetSample(int32 X, int32 Y);
	void SetSample(int32 X, int32 Y, float Value);
	void SetAlgorithms(const TArray<UHITerrainAlgorithm*>& InAlgorithms);
	int32 Size();

public:
	int32 GetChunkNums();
	void SetChunkNums(int32 InChunkNums);
	int32 GetChunkSize();
	void SetChunkSize(int32 InChunkSize);

public:
	OnDataGeneratedEvent OnDataGenerated;

protected:
	int32 GetIndex(int32 X, int32 Y, int32 TotalSize);

	void ApplyAlgorithm(UHITerrainAlgorithm* Algorithm);

protected:
	bool bIsGenerated = false;
	int32 ChunkNums;
	int32 ChunkSize;
	TArray<float> TerrainData;

	UPROPERTY()
	TArray<UHITerrainAlgorithm*> Algorithms;
};