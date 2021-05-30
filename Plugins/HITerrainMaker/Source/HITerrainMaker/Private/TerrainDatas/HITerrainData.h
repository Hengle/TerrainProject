#pragma once
#include "CoreMinimal.h"

#include "FoliageData.h"
#include "HITerrainCommons.h"
#include "HITerrainChunkData.h"
#include "HITerrainChannel.h"
#include "TerrainAlgorithms/HITerrainAlgorithm.h"


#include "HITerrainData.generated.h"

DECLARE_DELEGATE(OnDataGeneratedEvent)

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

	int32 RealSize();

	/*
	 * 获取地形的中心点
	 */
	FVector2D GetCenterPoint();

	/*
	 * 获取、修改索引为[X, Y]的采样点高度值
	 * 获取位置为[X, Y]的采样点高度值
	 * 这里的实现相当于获取height通道的float值。
	 */
	float GetHeightValue(int32 X, int32 Y);
	void SetHeightValue(int32 X, int32 Y, float Value);
	float GetHeightValue(float X, float Y);

	float GetSedimentValue(float X, float Y);
	float GetSedimentValue(int32 X, int32 Y);
	float GetWaterValue(float X, float Y);
	float GetWaterValue(int32 X, int32 Y);

	/*
	 * 添加指定名称、类型的通道。
	 */
	void AddChannel(FString ChannelName, ETerrainDataType Type);

	void AddChannel(FString ChannelName, TSharedPtr<FHITerrainChannel> FromChannel);

	/*
	 * 删除指定名称的通道。
	 */
	void DeleteChannel(FString ChannelName);

	/*
	 * 复制指定名称的通道到另一通道。
	 */
	void CopyChannel(FString FromChannelName, FString ToChannelName);

	/*
	 * 获取指定名称的通道。
	 */
	TSharedPtr<FHITerrainChannel> GetChannel(FString ChannelName);
	bool ContainsChannel(FString ChannelName);

	bool GetChannelValue(FString ChannelName, int32 X, int32 Y, float& Value);
	bool GetChannelValue(FString ChannelName, float X, float Y, float& Value);
	bool SetChannelValue(FString ChannelName, int32 X, int32 Y, const float& Value);

	bool GetChannelValue(FString ChannelName, int32 X, int32 Y, FVector& Value);
	bool SetChannelValue(FString ChannelName, int32 X, int32 Y, const FVector& Value);

	bool GetChannelValue(FString ChannelName, int32 X, int32 Y, bool& Value);
	bool SetChannelValue(FString ChannelName, int32 X, int32 Y, bool Value);

	bool GetChannelValue(FString ChannelName, int32 X, int32 Y, FQuat& Value);
	bool SetChannelValue(FString ChannelName, int32 X, int32 Y, const FQuat& Value);
	
	/*
	 * 获取区块数目（长宽）
	 */
	int32 GetChunkNums();
	
	/*
	 * 获取区块大小（长宽）
	 */
	int32 GetChunkSize();

	void AddChunkGrass(TPair<int32, int32>& Index, FVector& GrassPosition);

	void AddChunkFoliage(TPair<int32, int32>& Index, FFoliageData& Data);

	FRotator GetRotatorAtLocation(const FVector& Location);

	TArray<FVector>& GetChunkGrass(TPair<int32, int32>& Index);

	TArray<FFoliageData>& GetChunkFoliage(TPair<int32, int32>& Index);
	
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

	FCriticalSection Mutex;
	bool bAvailable = true;

protected:
	int32 GetIndex(int32 X, int32 Y, int32 InTotalSize);

	void ApplyAlgorithm(UHITerrainAlgorithm* Algorithm);

protected:
	bool bIsGenerated = false;
	int32 ChunkNums;
	int32 ChunkSize;

	int32 TotalSize;
	int32 RealTotalSize;
	
	TMap<FString, TSharedPtr<FHITerrainChannel>> TerrainDataChannels;
	
	UPROPERTY()
	TArray<UHITerrainAlgorithm*> Algorithms;

	FTerrainInformationPtr Information;

	TMap<TPair<int32, int32>, TArray<FVector>> GrassData;
	TMap<TPair<int32, int32>, TArray<FFoliageData>> FoliageData;
};