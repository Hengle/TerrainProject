#pragma once
#include "CoreMinimal.h"
#include "HITerrainCommons.h"
#include "HITerrainChunkData.h"
#include "TerrainAlgorithms/HITerrainAlgorithm.h"

#include "HITerrainData.generated.h"

DECLARE_DELEGATE(OnDataGeneratedEvent)

struct FTerrainSample
{
public:
	FTerrainSample():Value(0.0f), Type(ESampleType::NONE){}
	
	float Value;
	ESampleType Type;
};

UCLASS()
class UHITerrainData: public UObject, public FRunnable
{
	GENERATED_BODY()

public:
	virtual void InitData(){};
	virtual uint32 Run();
	
	virtual FChunkDataPtr GetChunkData(const TPair<int32, int32>& Index);
	float GetSampleValue(int32 X, int32 Y);
	void SetSampleValue(int32 X, int32 Y, float Value);
	void SetSampleType(int32 X, int32 Y, ESampleType Type);
	void SetAlgorithms(const TArray<UHITerrainAlgorithm*>& InAlgorithms);
	void SetInformation(FTerrainInformationPtr InInformation);
	int32 Size();
	FVector2D GetCenterPoint();

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
	TArray<FTerrainSample> TerrainData;

	UPROPERTY()
	TArray<UHITerrainAlgorithm*> Algorithms;

	FTerrainInformationPtr Information;
};