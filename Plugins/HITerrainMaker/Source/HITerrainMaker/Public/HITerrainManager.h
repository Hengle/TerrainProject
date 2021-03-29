#pragma once
#include "CoreMinimal.h"
#include "HITerrainCommons.h"
#include "HITerrainManager.generated.h"

/*
 * 插件的Manager类
 * 目前就是主要用于生成TerrainInstance。
 */
UCLASS()
class HITERRAINMAKER_API UHITerrainManager: public UObject
{
	GENERATED_BODY()

public:
	static UHITerrainManager* Get();

public:
	/*
	 * 创建TerrainInstance
	 */
	class AHITerrainInstance* CreateTerrainInstance(UObject* WorldContextObject, FTerrainInformationPtr TerrainInformation);


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

