#include "HITerrainActor.h"

void AHITerrainActor::Initialize(UHITerrainProviderBase* Provider)
{
	GetRuntimeMeshComponent()->Initialize(Provider);
}