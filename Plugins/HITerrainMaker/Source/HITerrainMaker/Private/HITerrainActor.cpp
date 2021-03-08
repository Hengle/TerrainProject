#include "HITerrainActor.h"

void AHITerrainActor::Initialize(UHITerrainProviderBase* Provider)
{
	check(Provider);
	GetRuntimeMeshComponent()->Initialize(Provider);
}