#pragma once
#include "CoreMinimal.h"
#include "RuntimeMeshCore.h"
#include "HITerrainCommons.generated.h"

DECLARE_LOG_CATEGORY_CLASS(LogHITerrain, Log, All)

const float FLAT_CHUNK_INTERVAL = 0.1;
const float FLAT_RENDER_DISTANCE = 50001;
const int32 FLAT_RENDER_CHUNKNUM = 10;
const float FLAT_CHUNK_SIZE = 5000;
const float FLAT_VERTICE_SIZE_HIGH = 25;
const float FLAT_VERTICE_SIZE_MEDIUM = 50;
const float FLAT_VERTICE_SIZE_LOW = 100;

UENUM()
enum class ETerrainType: uint8
{
	NONE = 0,
	SAMPLE,
};

UENUM()
enum class ELODLevel: uint8
{
	NONE = 0,
	LOD_LOW,
	LOD_MEDIUM,
	LOD_HIGH,
};

UENUM()
enum class ESampleType: uint8
{
	NONE = 0,
	GRASS,
	MOUNTAIN,
	RIVER,
	SEA,
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Information")
	ETerrainType TerrainType = ETerrainType::SAMPLE;	//地形种类

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Information")
	FVector Position = FVector(0.0, 0.0, 0.0);	// 地形位置（左下角点）

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Information")
	int32 ChunkNum = 20;		// 地形长宽（区块个数）

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Information")
	float ChunkSize = 5000;	// 区块大小

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Information")
	int32 Seed = 11111;			// 地形随机数种子

	/*
	 * 样例生成信息
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sample Generate Information")
	float ContinentFrequency = 1.0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sample Generate Information")
	float ContinentLacunarity = 2.208984375;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sample Generate Information")
	float SeaLevel = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sample Generate Information")
	float TerrainOffset = 1.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sample Generate Information")
	float ShelfLevel = -0.375;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sample Generate Information")
	float MountainLacunarity = 2.142578125;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sample Generate Information")
	float MountainsTwist = 1.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sample Generate Information")
	float MountainGlaciation = 1.375;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sample Generate Information")
	float HillsLacunarity = 2.162109375;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sample Generate Information")
	float HillsTwist = 1.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sample Generate Information")
	float PlainsLacunarity = 2.314453125;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sample Generate Information")
	float BadLandsLacunarity = 2.212890625;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sample Generate Information")
	float BadLandsTwist = 1.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sample Generate Information")
	float ContinentHeightScale = 0.25;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sample Generate Information")
	float HillsAmount = 0.75;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sample Generate Information")
	float MountainsAmount = 0.5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sample Generate Information")
	float BadlandsAmount = 0.03125;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sample Generate Information")
	float RiverDepth = 0.0234375;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sample Generate Information")
	float MaxElev = 8192.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sample Generate Information")
	float MinElev = -8192.0;

	/************************************************************************/
	/* 区块信息                                                               */
	/************************************************************************/
	

	/************************************************************************/
	/* 渲染信息                                                               */
	/************************************************************************/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Render Information")
	int32 RenderDistance = 21;	// 渲染区块范围
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Render Information")
	float LODHighQuality = 25;	// LOD高质量

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Render Information")
	float LODMediumQuality = 50;	// LOD中质量

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Render Information")
	float LODLowQuality = 100;	// LOD低质量

	/*
	 * 测试信息
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test Information")
	bool bEnableDebugAlgorithm = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test Information")
	bool bEnableLOD = true;
	 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test Information")
	float TEST_VORONOI_FREQUENCY = 1.0;
};

typedef TSharedPtr<FTerrainInformation> FTerrainInformationPtr; 

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