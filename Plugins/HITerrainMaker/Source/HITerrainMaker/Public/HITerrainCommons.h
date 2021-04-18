#pragma once
#include "CoreMinimal.h"
#include "RuntimeMeshCore.h"
#include "HITerrainCommons.generated.h"

DECLARE_LOG_CATEGORY_CLASS(LogHITerrain, Log, All)

UENUM()
enum class ETerrainType: uint8
{
	NONE = 0,
	PERLIN,
	VORONOI,
	RIDGED_MULTI,
	TEST,
};

UENUM()
enum class ELODLevel: uint8
{
	NONE = 0,
	LOD_LOW,
	LOD_MEDIUM,
	LOD_HIGH,
};

const int32 LOD_LOW_DISTANCE = 2;
const int32 LOD_MEDIUM_DISTANCE = 6;
const int32 LOD_HIGH_DISTANCE = 10;

UENUM()
enum class ESampleType: uint8
{
	NONE = 0,
	MARK_GROUND,
	MARK_WATER,
	MARK_NEARWATER,
	BEACH,
	GRASS,
	MOUNTAIN,
	OCEAN,
	LAKE,
	SNOWY,
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
	ETerrainType TerrainType = ETerrainType::PERLIN; //地形种类

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Information")
	FVector Position = FVector(0.0, 0.0, 0.0); // 地形位置（左下角点）

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Information")
	int32 ChunkNum = 20; // 地形长宽（区块个数）

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Information")
	float ChunkSize = 5000; // 区块大小

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Information")
	int32 Seed = 11111; // 地形随机数种子

	/*
	* 基本生成算法信息
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Generate Information")
	float BG_SeaLevel = 0.0f; // 海平面位置

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Generate Information")
	float BG_SandLevel = 100.0f; // 沙子材质位置

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Generate Information")
	float BG_GrassLevel = 200.0f; // 草材质位置

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Generate Information")
	float BG_MountainLevel = 500.0f; // 山峰材质位置

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Generate Information")
	float BG_LandscapeFrequency = 0.2f; // 地形噪声频率

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Generate Information")
	float BG_LandscapeLacunarity = 2.0f; // 地形噪声间隔系数

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Generate Information")
	int32 BG_LandscapeOctaveCount = 14; // 地形噪声间隔次数


	/*
	 * 样例生成信息
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sample Generate Information")
	float SG_ContinentFrequency = 1.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sample Generate Information")
	float SG_ContinentLacunarity = 2.208984375;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sample Generate Information")
	float SG_SeaLevel = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sample Generate Information")
	float SG_TerrainOffset = 1.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sample Generate Information")
	float SG_ShelfLevel = -0.375;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sample Generate Information")
	float SG_MountainLacunarity = 2.142578125;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sample Generate Information")
	float SG_MountainsTwist = 1.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sample Generate Information")
	float SG_MountainGlaciation = 1.375;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sample Generate Information")
	float SG_HillsLacunarity = 2.162109375;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sample Generate Information")
	float SG_HillsTwist = 1.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sample Generate Information")
	float SG_PlainsLacunarity = 2.314453125;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sample Generate Information")
	float SG_BadLandsLacunarity = 2.212890625;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sample Generate Information")
	float SG_BadLandsTwist = 1.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sample Generate Information")
	float SG_ContinentHeightScale = 0.25;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sample Generate Information")
	float SG_HillsAmount = 0.75;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sample Generate Information")
	float SG_MountainsAmount = 0.5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sample Generate Information")
	float SG_BadlandsAmount = 0.03125;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sample Generate Information")
	float SG_RiverDepth = 0.0234375;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sample Generate Information")
	float SG_MaxElev = 8192.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sample Generate Information")
	float SG_MinElev = -8192.0;

	/*
	 * 岛屿生成算法信息
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Island Generate Information")
	float IG_SeaLevel = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Island Generate Information")
	int32 IG_VoronoiCount = 400;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Island Generate Information")
	int32 IG_OceanWrapperCount = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Island Generate Information")
	float IG_OceanThreshold = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Island Generate Information")
	float IG_MainlandFrequency = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Island Generate Information")
	float IG_MainlandLacunarity = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Island Generate Information")
	int32 IG_MainlandOctave = 14;


	/************************************************************************/
	/* 渲染信息                                                               */
	/************************************************************************/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Render Information")
	int32 RenderDistance = 21; // 渲染区块范围

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Render Information")
	float ChunkGenerateInterval = 0.05; // 区块生成间隔

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Render Information")
	float LODHighQuality = 100; // LOD高质量

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Render Information")
	float LODMediumQuality = 200; // LOD中质量

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Render Information")
	float LODLowQuality = 500; // LOD低质量

	/*
	 * 测试信息
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test Information")
	bool bEnableDebugAlgorithm = true; // 算法使用Debug版本

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test Information")
	bool bEnableLOD = true; // 使用LOD

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test Information")
	float TEST_VORONOI_FREQUENCY = 1.0;
};

typedef TSharedPtr<FTerrainInformation, ESPMode::ThreadSafe> FTerrainInformationPtr;

UCLASS()
class UHITerrainCommon : public UObject
{
	GENERATED_BODY()

public:
	
};
