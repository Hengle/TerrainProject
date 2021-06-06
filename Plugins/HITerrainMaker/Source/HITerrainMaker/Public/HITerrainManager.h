#pragma once
#include "CoreMinimal.h"
#include "HITerrainCommons.h"
#include "HITerrainManager.generated.h"

/*
 * 插件的Manager类
 * 目前就是主要用于生成TerrainInstance。
 */
UCLASS(BlueprintType)
class HITERRAINMAKER_API UHITerrainManager: public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	static UHITerrainManager* Get();

public:
	/*
	 * 创建TerrainInstance
	 */
	class AHITerrainInstance* CreateTerrainInstance(UObject* WorldContextObject, FTerrainInformationPtr TerrainInformation);

	UFUNCTION(BlueprintCallable)
	class AHITerrainInstance* CreateTerrainInstance(UObject* WorldContextObject, FTerrainInformation TerrainInformation);

	UFUNCTION(BlueprintCallable)
	void DeleteTerrainInstance(AHITerrainInstance* TerrainInstance);
	
public:
	/*
	 * 获取玩家位置
	 */
	FVector GetPlayerLocation(UObject* WorldContextObject)
	{
		return WorldContextObject->GetWorld()->GetFirstPlayerController()->GetPawn()->GetActorLocation();
	}

private:
	UHITerrainManager(){};

	static UHITerrainManager* Instance;
};

