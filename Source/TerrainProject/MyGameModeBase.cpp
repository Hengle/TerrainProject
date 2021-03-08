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
	FTerrainInformation TerrainInformation;
	TerrainInformation.ChunkNum = ChunkNum;
	TerrainInformation.Position = Position;
	TerrainInformation.Seed = Seed;
	TerrainInstance = TerrainManager->CreateTerrainInstance(this, TerrainInformation);
}