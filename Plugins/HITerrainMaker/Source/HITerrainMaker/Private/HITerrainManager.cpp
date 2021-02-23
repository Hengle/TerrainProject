#include "HITerrainManager.h"
#include "HITerrainInstance.h"

UHITerrainManager* UHITerrainManager::Instance = nullptr;

AHITerrainInstance* UHITerrainManager::CreateTerrainInstance(UWorld* World, const FTerrainInformation& TerrainInformation) {
	//AHITerrainInstance* TerrainInstance = NewObject<AHITerrainInstance>();
	AHITerrainInstance* TerrainInstance = Cast<AHITerrainInstance>(World->SpawnActor(AHITerrainInstance::StaticClass(), &TerrainInformation.Position));
	TerrainInstance->Init(TerrainInformation);
	return TerrainInstance;
}