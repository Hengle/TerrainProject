// Copyright Epic Games, Inc. All Rights Reserved.

#include "TerrainProjectGameMode.h"
#include "TerrainProjectCharacter.h"
#include "UObject/ConstructorHelpers.h"

ATerrainProjectGameMode::ATerrainProjectGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}

void ATerrainProjectGameMode::BeginPlay()
{
	InitDataManager();
	float ActorInterval = ActorParameter.SectionInterval * ActorParameter.SectionNumX * ActorParameter.SectionNumY;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &ATerrainProjectGameMode::CreateTerrainActor, ActorInterval, true, 0);
}

void ATerrainProjectGameMode::CreateTerrainActor() {
	if (ActorIndex < ActorNumX * ActorNumY) {
		FVector Location = FVector(0, 0, 0);
		ATerrainActor* TerrainActor = Cast<ATerrainActor>(GetWorld()->SpawnActor(ATerrainActor::StaticClass(), &Location));
		TerrainActor->Param = ActorParameter;
		TerrainActor->Param.ActorIndexX = ActorRecentX;
		TerrainActor->Param.ActorIndexY = ActorRecentY;
		TerrainActor->TaskParam = TaskParameter;
		TerrainActor->Start();
		TerrainActors.Add(TerrainActor);
		UE_LOG(LogTemp, Warning, TEXT("Terrain Actor Created! X:%d, Y:%d"), ActorRecentX, ActorRecentY)
		ActorIndex++;
		ActorRecentY++;
		if (ActorRecentY >= ActorNumY) {
			ActorRecentY = 0;
			ActorRecentX++;
		}
	}
	
}

void ATerrainProjectGameMode::InitDataManager() {
	TerrainDataManager& Manager = TerrainDataManager::GetInstance();
	Manager.SetSeed(Seed);
	Manager.SetLocationScale(LocationScale);
	Manager.SetHeightScale(HeightScale);
}