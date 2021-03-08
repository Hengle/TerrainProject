#include "HITerrainManager.h"
#include "HITerrainInstance.h"

UHITerrainManager* UHITerrainManager::Instance = nullptr;

void CreateTerrain(UObject* WorldContextObject, const FTerrainInformation& TerrainInformation) 
{
	
}

//AHITerrainInstance* UHITerrainManager::CreateTerrainInstance(UWorld* World, const FTerrainInformation& TerrainInformation) {
//	//AHITerrainInstance* TerrainInstance = NewObject<AHITerrainInstance>();
//	AHITerrainInstance* TerrainInstance = Cast<AHITerrainInstance>(World->SpawnActor(AHITerrainInstance::StaticClass(), &TerrainInformation.Position));
//	TerrainInstance->Init(TerrainInformation);
//	return TerrainInstance;
//}

UHITerrainManager* UHITerrainManager::Get()
{
	if (Instance == nullptr)
	{
		Instance = NewObject<UHITerrainManager>();
		Instance->AddToRoot();
	}
	return Instance;
}