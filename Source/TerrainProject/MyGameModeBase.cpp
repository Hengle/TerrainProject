#include "MyGameModeBase.h"
#include "HITerrainManager.h"
#include "HITerrainInstance.h"


void AMyGameModeBase::BeginPlay()
{
	CreateTerrain();
}

void AMyGameModeBase::CreateTerrain() 
{
	UHITerrainManager* TerrainManager = UHITerrainManager::Get();
	TerrainInstance = TerrainManager->CreateTerrainInstance(this, MakeShared<FTerrainInformation>());
	TerrainInstance->Material = Material;
}