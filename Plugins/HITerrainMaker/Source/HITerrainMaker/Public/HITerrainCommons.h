#pragma once
#include "CoreMinimal.h"
#include "RuntimeMeshCore.h"
#include "HITerrainCommons.generated.h"

DECLARE_LOG_CATEGORY_CLASS(LogHITerrain, Log, All)

#define FENCE	AsyncTask(ENamedThreads::GameThread, []()	\
				{											\
					FRenderCommandFence Fence;				\
					Fence.BeginFence();						\
					Fence.Wait();							\
				});											

struct FTerrainRWBufferStructured
{
	FStructuredBufferRHIRef Buffer;
	FUnorderedAccessViewRHIRef UAV;
	FShaderResourceViewRHIRef SRV;
	uint32 NumBytes;

	FTerrainRWBufferStructured(): NumBytes(0) {}

	~FTerrainRWBufferStructured()
	{
	}

	void Initialize(uint32 BytesPerElement, uint32 NumElements, FRHIResourceCreateInfo CreateInfo, uint32 AdditionalUsage = 0, bool bUseUavCounter = false, bool bAppendBuffer = false)
	{
		check(GMaxRHIFeatureLevel == ERHIFeatureLevel::SM5 || GMaxRHIFeatureLevel == ERHIFeatureLevel::ES3_1);
		ensure(!((AdditionalUsage & BUF_FastVRAM)));

		NumBytes = BytesPerElement * NumElements;
		Buffer = RHICreateStructuredBuffer(BytesPerElement, NumBytes, BUF_UnorderedAccess | BUF_ShaderResource | AdditionalUsage, ERHIAccess::UAVMask, CreateInfo);
		UAV = RHICreateUnorderedAccessView(Buffer, bUseUavCounter, bAppendBuffer);
		SRV = RHICreateShaderResourceView(Buffer);
	}

	void Release()
	{
		int32 BufferRefCount = Buffer ? Buffer->GetRefCount() : -1;

		if (BufferRefCount == 1)
		{
			DiscardTransientResource();
		}

		NumBytes = 0;
		Buffer.SafeRelease();
		UAV->SetCommitted(false);
		UAV.SafeRelease();
		SRV.SafeRelease();
	}

	void AcquireTransientResource()
	{
		RHIAcquireTransientResource(Buffer);
	}
	void DiscardTransientResource()
	{
		RHIDiscardTransientResource(Buffer);
	}
};

UENUM()
enum class ETerrainType: uint8
{
	NONE = 0,
	PERLIN,
	VORONOI,
	RIDGED_MULTI,
	TEST,
	TEST2,
	TEST3,
};

UENUM()
enum class ELODLevel: uint8
{
	NONE = 0,
	LOD_LOW,
	LOD_MEDIUM,
	LOD_HIGH,
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
	int32 ChunkNum = 10; // 地形长宽（区块个数）

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Information")
	float ChunkSize = 10000; // 区块大小

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Information")
	int32 RealTotalSize = 1024; // 实际大小，填充到一个比较好的2的倍数

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


	/*
	 * 侵蚀算法参数
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion Algorithm")
	int32 Erosion_IterationNum = 1200;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion Algorithm")
	float Erosion_DeltaTime = 0.02f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion Algorithm")
	bool Erosion_EnableHydroErosion = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion Algorithm")
	bool Erosion_EnableThermalErosion = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion Algorithm")
	float Erosion_RainScale = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion Algorithm")
	float Erosion_EvaporationScale = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion Algorithm")
	float Erosion_HydroErosionScale = 0.008f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion Algorithm")
	float Erosion_HydroDepositionScale = 0.016f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion Algorithm")
	float Erosion_ThermalErosionScale = 0.03f;

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
	float LODLowQuality = 400; // LOD低质量

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
