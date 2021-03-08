#pragma once
#include "CoreMinimal.h"
#include "RuntimeMeshCore.h"
#include "HITerrainCommons.generated.h"

DECLARE_LOG_CATEGORY_CLASS(LOGHITerrain, Log, All)

const float FLAT_CHUNK_INTERVAL = 0.1;
const float FLAT_RENDER_DISTANCE = 50001;
const int32 FLAT_RENDER_CHUNKNUM = 10;
const float FLAT_CHUNK_SIZE = 5000;
const float FLAT_VERTICE_SIZE_HIGH = 20;
const float FLAT_VERTICE_SIZE_MEDIUM = 50;
const float FLAT_VERTICE_SIZE_LOW = 100;

USTRUCT()
struct HITERRAINMAKER_API FTerrainInformation
{
	GENERATED_USTRUCT_BODY()

	FVector Position;
	int32 ChunkNum;
	int32 Seed;
	float Height;
};

/**
 * 一个Chunk的基本信息，即各采样点的高度。
 */
struct FChunkInformation
{
	// 采样点个数
	int32 SampleNums;
	// 采样点数据
	TArray<float> Samples;
	
	float GetSample(int32 X, int32 Y) 
	{
		if (X < 0 || X > SampleNums || Y < 0 || Y > SampleNums)
		{
			return 0.0f;
		}
		else 
		{
			return Samples[X * SampleNums + Y];
		}
	}

	void SetSample(int32 X, int32 Y, float Value) 
	{
		Samples[X * SampleNums + Y] = Value;
	}
};
typedef TSharedPtr<FChunkInformation, ESPMode::ThreadSafe> FChunkInformationPtr;

UCLASS()
class UHITerrainCommon: public UObject
{
	GENERATED_BODY()

public:
	static float Lerp2D(float LL, float LH, float HL, float HH, float LA, float HA) 
	{
		float L1 = Lerp(LL, LH, LA);
		float L2 = Lerp(HL, HH, LA);
		return Lerp(L1, L2, HA);
	}

	static float Lerp(float Low, float High, float Alpha) 
	{
		return Low * (1 - SmoothStep(Alpha)) + High * SmoothStep(Alpha);
	}

	static float SmoothStep(float Alpha) 
	{
		return Alpha * Alpha * (3 - 2 * Alpha);
	}
};