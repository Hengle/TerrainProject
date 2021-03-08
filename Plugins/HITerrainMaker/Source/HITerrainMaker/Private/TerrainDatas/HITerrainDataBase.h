#pragma once
#include "CoreMinimal.h"
#include "HITerrainCommons.h"

DECLARE_DELEGATE(OnDataGeneratedEvent)

class UHITerrainDataBase: public UObject, public FRunnable
{
public:
	virtual void InitData() = 0;
	virtual uint32 Run() = 0;
	virtual TSharedPtr<FChunkInformation> GetChunkData(const TPair<int32, int32>& Index);

public:
	void SetSeed(int32 InSeed);
	void SetChunkNums(int32 InChunkNums);
	void SetChunkSampleNums(int32 InChunkSampleNums);

public:
	OnDataGeneratedEvent OnDataGenerated;

protected:
	float GetSample(int32 X, int32 Y);
	void SetSample(int32 X, int32 Y, float Value);

protected:
	virtual void GenerateChunkData(int32 X, int32 Y);

protected:
	int32 Seed;
	int32 ChunkNums;
	int32 ChunkSampleNums;
	bool bIsGenerated = false;
	TMap<TPair<int32, int32>, FChunkInformationPtr> ChunkData;
};