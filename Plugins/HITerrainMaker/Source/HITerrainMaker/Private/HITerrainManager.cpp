#include "HITerrainManager.h"
#include "HITerrainInstance.h"

UHITerrainManager* UHITerrainManager::Instance = nullptr;

AHITerrainInstance* CreateTerrainInstance(UObject* WorldContextObject, const FTerrainInformation& TerrainInformation)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull)) 
	{
		AHITerrainInstance* TerrainInstance = Cast<AHITerrainInstance>(World->SpawnActor(AHITerrainInstance::StaticClass(), &TerrainInformation.Position));
		TerrainInstance->Init(TerrainInformation);
		return TerrainInstance;
	}
	else 
	{
		return nullptr;
	}
}

UHITerrainManager* UHITerrainManager::Get()
{
	if (Instance == nullptr)
	{
		Instance = NewObject<UHITerrainManager>();
		Instance->AddToRoot();
	}
	return Instance;
}