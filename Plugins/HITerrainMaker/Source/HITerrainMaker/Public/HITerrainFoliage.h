// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "HITerrainFoliage.generated.h"

/**
 * 
 */
UCLASS()
class HITERRAINMAKER_API UHITerrainFoliage : public UObject
{
	GENERATED_BODY()

public:
	static UHITerrainFoliage* Get();

	UStaticMesh* GetFoliageOfType(uint8 Type);

	UPROPERTY()
	UStaticMesh* StaticMeshBush01 = LoadObject<UStaticMesh>(this,TEXT("/Game/Light_Foliage/Meshes/SM_Bush_01"));
	
	UPROPERTY()
	UStaticMesh* StaticMeshBush02 = LoadObject<UStaticMesh>(this,TEXT("/Game/Light_Foliage/Meshes/SM_Bush_02"));
	
	UPROPERTY()
	UStaticMesh* StaticMeshBush03 = LoadObject<UStaticMesh>(this,TEXT("/Game/Light_Foliage/Meshes/SM_Bush_03"));
	
	UPROPERTY()
	UStaticMesh* StaticMeshBush04 = LoadObject<UStaticMesh>(this,TEXT("/Game/Light_Foliage/Meshes/SM_Bush_04"));

	UPROPERTY()
	UStaticMesh* StaticMeshBush05 = LoadObject<UStaticMesh>(this,TEXT("/Game/Light_Foliage/Meshes/SM_Bush_05"));

	UPROPERTY()
	UStaticMesh* StaticMeshGrass01 = LoadObject<UStaticMesh>(this,TEXT("/Game/Light_Foliage/Meshes/SM_Grass_01"));

	UPROPERTY()
	UStaticMesh* StaticMeshGrass02 = LoadObject<UStaticMesh>(this,TEXT("/Game/Light_Foliage/Meshes/SM_Grass_02"));
	
	UPROPERTY()
	UStaticMesh* StaticMeshGrass05 = LoadObject<UStaticMesh>(this,TEXT("/Game/Light_Foliage/Meshes/SM_Grass_05"));

	UPROPERTY()
	UStaticMesh* StaticMeshTree = LoadObject<UStaticMesh>(this,TEXT("/Game/KiteDemo/Environments/Trees/HillTree_Tall_02/HillTree_Tall_02"));

private:
	UHITerrainFoliage(){};

	static UHITerrainFoliage* Instance;
};
