#pragma once
#include "CoreMinimal.h"
#include "RuntimeMeshCore.h"
#include "HITerrainCommons.generated.h"

DECLARE_LOG_CATEGORY_CLASS(LOGHITerrain, Log, All)

const float FLAT_CHUNK_INTERVAL = 0.3;
const float FLAT_RENDER_DISTANCE = 50001;
const int32 FLAT_RENDER_CHUNKNUM = 10;
const float FLAT_CHUNK_SIZE = 5000;
const float FLAT_VERTICE_SIZE_HIGH = 25;
const float FLAT_VERTICE_SIZE_MEDIUM = 50;
const float FLAT_VERTICE_SIZE_LOW = 100;

UENUM()
enum class ETerrainType 
{
	NONE = 0,
	SAMPLE,
};

/**
 * 用于生成特定地形的信息。
 * 目标是通过这个地形信息，可以唯一地确定一个地形，不需要通过其他参数来调整地形。
 */
USTRUCT(BlueprintType)
struct HITERRAINMAKER_API FTerrainInformation
{
	GENERATED_USTRUCT_BODY()
	/************************************************************************/
	/* 基础信息                                                               */
	/************************************************************************/
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Basic Information")
	ETerrainType TerrainType = ETerrainType::SAMPLE;	//地形种类

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Basic Information")
	FVector Position = FVector(0.0, 0.0, 0.0);	// 地形位置（左下角点）

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Basic Information")
	int32 ChunkNum = 10;		// 地形长宽（区块个数）

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Basic Information")
	int32 Seed = 10086;			// 地形随机数种子

	/************************************************************************/
	/* 山峰、平原生成信息                                                       */
	/************************************************************************/
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Mountain/Plain Information")
	float MountainHeight = 2000;		// 山峰高度系数

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Mountain/Plain Information")
	float MountainScale = 0.1;		// 山峰位置系数

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Mountain/Plain Information")
	float PlainHeight = 100;		// 平原高度系数

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Mountain/Plain Information")
	float PlainScale = 1;		// 山峰位置系数

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Mountain/Plain Information")
	float PlainThreshold = 10;		// 平原阈值

	/************************************************************************/
	/* 区块信息                                                               */
	/************************************************************************/
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Chunk Information")
	float ChunkSize = 5000;	// 区块大小

	/************************************************************************/
	/* LOD信息                                                               */
	/************************************************************************/
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "LOD Information")
	float LODHighQuality = 25;	// LOD高质量

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "LOD Information")
	float LODMediumQuality = 50;	// LOD中质量

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "LOD Information")
	float LODLowQuality = 100;	// LOD低质量
};

/**
 * 一个Chunk的基本信息，即各采样点的高度。
 */
struct FChunkInformation
{
	// 采样点个数
	int32 SampleNums;
	// 采样点数据
	//TArray<float> Samples;
	TMap<TPair<int32, int32>, float> Samples;
	
	float GetSample(int32 X, int32 Y) 
	{
		if (X < 0 || X > SampleNums || Y < 0 || Y > SampleNums)
		{
			return 0.0f;
		}
		else 
		{
			return Samples[TPair<int32, int32>(X, Y)];
		}
	}

	void SetSample(int32 X, int32 Y, float Value) 
	{
		if (X < 0 || X > SampleNums || Y < 0 || Y > SampleNums)
		{
			UE_LOG(LOGHITerrain, Error, TEXT("FChunkInformation::SetSample Out Of Index! [%d, %d]"), X, Y)
		}
		else
		{
			Samples.Add(TPair<int32, int32>(X, Y), Value);
		}	
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