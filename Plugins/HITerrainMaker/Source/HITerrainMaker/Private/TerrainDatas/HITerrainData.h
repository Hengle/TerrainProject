#pragma once
#include "CoreMinimal.h"

#include "HITerrainCommons.h"
#include "HITerrainChunkData.h"
#include "HITerrainDataValue.h"
#include "TerrainAlgorithms/HITerrainAlgorithm.h"
#include "TerrainMaths/2DArray.h"


#include "HITerrainData.generated.h"

DECLARE_DELEGATE(OnDataGeneratedEvent)

/*
 * 单个地形采样点的数据
 */
struct FTerrainSample
{
public:
	FTerrainSample():Value(0.0f), Type(ESampleType::NONE){}
	
	float Value;	// 采样点高度
	ESampleType Type;	// 采样点类型（uint8） 要不要用采样点类型这样的设计，还是别的方法，目前还没想好。。。
};

/*
 * 地形数据类
 * 使用一个TFixed2DArray来保存一片地形的采样点数据
 * 负责保存、更改、提供地形采样点数据、进行地形采样点相关的操作。
 */
UCLASS()
class UHITerrainData: public UObject, public FRunnable
{
	GENERATED_BODY()

/*
 * Getters&Setters
 */
public:
	/*
	 * 获取一个区块的数据接口
	 */
	FChunkDataPtr GetChunkData(const TPair<int32, int32>& Index);

	/*
	 * 获取地形的大小（节点数）
	 */
	int32 Size();

	/*
	 * 获取地形的中心点
	 */
	FVector2D GetCenterPoint();

	/*
	 * 获取、修改索引为[X, Y]的采样点值
	 * 获取位置为[X, Y]的采样点值
	 */
	float GetSampleValue(int32 X, int32 Y);
	void SetSampleValue(int32 X, int32 Y, float Value);
	float GetSampleValue(float X, float Y);

	const TFixed2DArray<FHITerrainDataValue>& GetChannel(FString ChannelName);
	
	/*
	* 获取、修改索引为[X, Y]的采样点类型
	* 获取位置为[X, Y]的采样点类型
	*/
	ESampleType GetSampleType(int32 X, int32 Y);
	void SetSampleType(int32 X, int32 Y, ESampleType Type);
	ESampleType GetSampleType(float X, float Y);

	/*
	 * 获取区块数目（长宽）
	 */
	int32 GetChunkNums();
	
	/*
	 * 获取区块大小（长宽）
	 */
	int32 GetChunkSize();
	
/*
 * 内部用public
 */
public:
	virtual uint32 Run();

	OnDataGeneratedEvent OnDataGenerated;

	void SetChunkNums(int32 InChunkNums);

	void SetChunkSize(int32 InChunkSize);

	void SetAlgorithms(const TArray<UHITerrainAlgorithm*>& InAlgorithms);
	
	void SetInformation(FTerrainInformationPtr InInformation);

protected:
	int32 GetIndex(int32 X, int32 Y, int32 TotalSize);

	void ApplyAlgorithm(UHITerrainAlgorithm* Algorithm);

protected:
	bool bIsGenerated = false;
	int32 ChunkNums;
	int32 ChunkSize;
	TFixed2DArray<FTerrainSample> TerrainData;
	
	TMap<FString, TFixed2DArray<FHITerrainDataValue>> TerrainDataChannels;
	
	UPROPERTY()
	TArray<UHITerrainAlgorithm*> Algorithms;

	FTerrainInformationPtr Information;
};