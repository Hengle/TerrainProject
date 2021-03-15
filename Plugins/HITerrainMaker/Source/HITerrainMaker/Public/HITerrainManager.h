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
	class AHITerrainInstance* CreateTerrainInstance(UObject* WorldContextObject, FTerrainInformationPtr TerrainInformation);


public:
	FVector GetPlayerLocation(UObject* WorldContextObject)
	{
		return WorldContextObject->GetWorld()->GetFirstPlayerController()->GetPawn()->GetActorLocation();
	}

private:
	UHITerrainManager(){};

	static UHITerrainManager* Instance;
};

