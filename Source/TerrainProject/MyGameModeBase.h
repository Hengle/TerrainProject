// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "HITerrainCommons.h"
#include "Blueprint/UserWidget.h"
#include "MyGameModeBase.generated.h"

/**
 * 
 */
UCLASS()
class TERRAINPROJECT_API AMyGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	void CreateTerrain();

	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerrainInformation")
	FTerrainInformation TerrainInformation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerrainInformation")
	UMaterialInterface* Material;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerrainInformation")
	UMaterialInterface* WaterMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UMG")
	TSubclassOf<UUserWidget> StartingWidgetClass;

	UPROPERTY()
	UUserWidget* CurrentWidget;

private:
	UPROPERTY()
	class AHITerrainInstance* TerrainInstance;
};
