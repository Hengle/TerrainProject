#pragma once
#include "CoreMinimal.h"
#include "HITerrainCommons.h"
#include "RuntimeMeshActor.h"
#include "Providers/HITerrainProviderBase.h"
#include "HITerrainActor.generated.h"

UCLASS()
class AHITerrainActor : public ARuntimeMeshActor 
{
	GENERATED_BODY()

public:
	void Initialize(UHITerrainProviderBase* Provider);

private:
};