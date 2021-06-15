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

#define LOCK	while(!Data->bAvailable)					\
				{											\
					FPlatformProcess::Sleep(0.005f);		\
				}											\
				Data->bAvailable = false;

#define UNLOCK	Data->bAvailable = true;

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
		Buffer->SetCommitted(false);
		Buffer.SafeRelease();
		UAV->SetCommitted(false);
		UAV.SafeRelease();
		SRV->SetCommitted(false);
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

UENUM(BlueprintType)
enum class ETerrainType: uint8
{
	NONE = 0		UMETA(DisplayName = "无"),
	PERLIN			UMETA(DisplayName = "中期1"),
	VORONOI			UMETA(DisplayName = "中期2"),
	RIDGED_MULTI	UMETA(DisplayName = "中期3"),
	TEST			UMETA(DisplayName = "测试1"),
	FINAL			UMETA(DisplayName = "结题"),
	TEST3			UMETA(DisplayName = "测试2"),
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
	ETerrainType TerrainType = ETerrainType::FINAL; //地形种类

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Information")
	FVector Position = FVector(0.0, 0.0, 0.0); // 地形位置（左下角点）

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Information")
	int32 ChunkNum = 10; // 地形长宽（区块个数）

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Information")
	float ChunkSize = 10000; // 区块大小

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Information")
	int32 RealTotalSize = 1024; // 实际大小，填充到一个比较好的2的倍数

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Information")
	int32 Seed = 19999; // 地形随机数种子

	/*
	 * 基础地形参数
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Terrain Algorithm")
	float Terrain_Amplitude = 4000.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Terrain Algorithm")
	float Terrain_Scale = 0.002f;

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
	float Erosion_HydroErosionScale = 0.024f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion Algorithm")
	float Erosion_HydroDepositionScale = 0.016f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion Algorithm")
	float Erosion_ThermalErosionScale = 0.3f;

	/*
	 * 植被生成信息
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EcoSystem Algorithm")
	bool EcoSystem_bEnableFoliage = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EcoSystem Algorithm")
	float EcoSystem_GrassAmount = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EcoSystem Algorithm")
	float EcoSystem_TreeAmount = 0.005f;

	/************************************************************************/
	/* 渲染信息                                                               */
	/************************************************************************/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Render Information")
	int32 RenderDistance = 21; // 渲染区块范围

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Render Information")
	float ChunkGenerateInterval = 0.1; // 区块生成间隔

	/*
	 * 测试信息
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test Information")
	bool bEnableDebugAlgorithm = true; // 算法使用Debug版本

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test Information")
	bool bEnableLOD = true; // 使用LOD
};

typedef TSharedPtr<FTerrainInformation, ESPMode::ThreadSafe> FTerrainInformationPtr;
