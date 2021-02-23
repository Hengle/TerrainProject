#pragma once
#include "CoreMinimal.h"
#include "HITerrainCommons.h"
#include "HITerrainManager.generated.h"

UCLASS()
class HITERRAINMAKER_API UHITerrainManager: public UObject
{
	GENERATED_BODY()

public:
	static UHITerrainManager* Get() {
		if (Instance == nullptr) {
			Instance = NewObject<UHITerrainManager>();
			Instance->AddToRoot();
		}
		return Instance;
	}

public:
	class AHITerrainInstance* CreateTerrainInstance(UWorld* World, const FTerrainInformation& TerrainInformation);

public:
	FVector GetPlayerLocation(UWorld* World) {
		return World->GetFirstPlayerController()->GetPawn()->GetActorLocation();
	}

private:

	static UHITerrainManager* Instance;
};

