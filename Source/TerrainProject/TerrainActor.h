// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "noiselib/noise.h"
#include "TerrainUtils.h"
#include "TerrainActor.generated.h"

UCLASS()
class TERRAINPROJECT_API ATerrainActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATerrainActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void Start();

	void CreateTerrainMesh();


public:
	FTerrainActorParameter Param;

	FTerrainTaskParameter TaskParam;

private:
	UPROPERTY()
	UProceduralMeshComponent* ProceduralMesh;


private:
	int32 RecentX = 0;
	
	int32 RecentY = 0;

	int32 Index = 0;

	FTimerHandle TimerHandle;
};
