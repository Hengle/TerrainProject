#include "HITerrainManager.h"
#include "HITerrainInstance.h"

UHITerrainManager* UHITerrainManager::Instance = nullptr;

AHITerrainInstance* UHITerrainManager::CreateTerrainInstance(UObject* WorldContextObject, FTerrainInformationPtr TerrainInformation)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull)) 
	{
		AHITerrainInstance* TerrainInstance = Cast<AHITerrainInstance>(World->SpawnActor(AHITerrainInstance::StaticClass(), &TerrainInformation->Position));
		TerrainInstance->Init(TerrainInformation);
		return TerrainInstance;
	}
	else 
	{
		return nullptr;
	}
}

AHITerrainInstance* UHITerrainManager::CreateTerrainInstance(UObject* WorldContextObject, FTerrainInformation TerrainInformation)
{
	return CreateTerrainInstance(WorldContextObject, MakeShareable<FTerrainInformation>(new FTerrainInformation(TerrainInformation)));
}

void UHITerrainManager::DeleteTerrainInstance(AHITerrainInstance* TerrainInstance)
{
	if(TerrainInstance != nullptr)
	{
		TerrainInstance->Destroy();
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
