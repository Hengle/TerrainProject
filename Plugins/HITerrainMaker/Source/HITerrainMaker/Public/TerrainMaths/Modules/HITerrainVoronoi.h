// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "HITerrainModule.h"
#include "Runtime/Experimental/Voronoi/Public/Voronoi/Voronoi.h"
#include "TerrainMaths/2DArray.h"

class HITERRAINMAKER_API FHITerrainVoronoi: public FHITerrainModule
{
public:
	void Init(int32 InSeed, float InSizeX, float InSizeY, int32 InNumSites);

	void SetTargetChannel(const FString& InChannelName);
	void SetSeed(int32 InSeed);
	void SetNumSites(int32 InNumSites);
	void SetAmplitude(float InAmplitude);

	virtual void ApplyModule(UHITerrainData* Data) override;
	 
	float GetCellValue(float X, float Y);

	const TArray<FVoronoiCellInfo>& GetAllCells();

	int32 Position2Cell(float X, float Y);

	int32 Position2Cell(FVector Position);

	const TArray<FVector>& GetSites();

	 ~FHITerrainVoronoi();
	
private:
	FString ChannelName;
	int32 Seed;
	float NumSites;
	float Amplitude;
	FBox Bounds;
	TArray<FVector> Sites;
	TArray<FVoronoiCellInfo> AllCells;
	FVoronoiDiagram* VoronoiDiagram = nullptr;
};
