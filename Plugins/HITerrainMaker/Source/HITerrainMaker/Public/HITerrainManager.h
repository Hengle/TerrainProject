#pragma once
#include "CoreMinimal.h"
#include "HITerrainCommons.h"
#include "HITerrainManager.generated.h"

UCLASS()
class HITERRAINMAKER_API UHITerrainManager: public UObject
{
	GENERATED_BODY()

public:
	static UHITerrainManager* Get();

public:
	//class AHITerrainInstance* CreateTerrainInstance(UWorld* World, const FTerrainInformation& TerrainInformation);

	void CreateTerrain(UObject* WorldContextObject, const FTerrainInformation& TerrainInformation);


public:
	FVector GetPlayerLocation(UObject* WorldContextObject)
	{
		return WorldContextObject->GetWorld()->GetFirstPlayerController()->GetPawn()->GetActorLocation();
	}

private:
	UHITerrainManager();

	static UHITerrainManager* Instance;
};

