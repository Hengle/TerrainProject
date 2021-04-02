/*
* This is an independent project of an individual developer. Dear PVS-Studio, please check it.
* PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
*/
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

UHITerrainManager* UHITerrainManager::Get()
{
	if (Instance == nullptr)
	{
		Instance = NewObject<UHITerrainManager>();
		Instance->AddToRoot();
	}
	return Instance;
}