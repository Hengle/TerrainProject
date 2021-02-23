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
	TerrainInformation.Material = Material;
	TerrainInformation.Position = Position;
	TerrainInformation.HeightScale = HeightScale;
	TerrainInformation.PositionScale = PositionScale;
	TerrainInformation.Seed = Seed;
	TerrainInformation.TerrainNoiseType = TerrainNoiseType;
	TerrainInformation.TerrainType = TerrainType;
	TerrainInstance = TerrainManager->CreateTerrainInstance(GetWorld(), TerrainInformation);
}