// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainActor.h"
#include "TerrainMeshSectionTask.h"
#include "HAL/RunnableThread.h"

// Sets default values
ATerrainActor::ATerrainActor() {
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("GeneratedMesh"));
	RootComponent = ProceduralMesh;
	//New in UE 4.17, multi - threaded PhysX cooking.
	ProceduralMesh->bUseAsyncCooking = true;
}

// Called when the game starts or when spawned
void ATerrainActor::BeginPlay()
{
	Super::BeginPlay();
	//GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &ATerrainActor::CreateTerrainMesh, Param.SectionInterval, true, Param.SectionInterval);
}

// Called every frame
void ATerrainActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ATerrainActor::Start() {
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &ATerrainActor::CreateTerrainMesh, Param.SectionInterval, true, Param.SectionInterval);
}

void ATerrainActor::CreateTerrainMesh() {
	if (Index < Param.SectionNumX * Param.SectionNumY) {
		FTerrainMeshSectionParameter Parameter;
		Parameter.SectionIndex = Index;
		Parameter.XIndex = RecentX + Param.ActorIndexX * Param.SectionNumX;
		Parameter.YIndex = RecentY + Param.ActorIndexY * Param.SectionNumY;
		Parameter.TaskParam = TaskParam;
		FTerrainMeshSectionTask* Task = new FTerrainMeshSectionTask(ProceduralMesh, Parameter);
		Task->CreateTerrainMeshSection();
		Index++;
		RecentY++;
		if (RecentY >= Param.SectionNumY) {
			RecentY = 0;
			RecentX++;
		}
	}
	
}