// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Runtime/Experimental/Voronoi/Public/Voronoi/Voronoi.h"
#include "TerrainMaths/2DArray.h"

class HITERRAINMAKER_API FHITerrainVoronoi
{
public:
	void Init(int32 InSeed, float InSizeX, float InSizeY, int32 InNumSites);
	 
	float GetCellValue(float X, float Y);

	const TArray<FVoronoiCellInfo>& GetAllCells();

	int32 Position2Cell(float X, float Y);

	int32 Position2Cell(FVector Position);

	const TArray<FVector>& GetSites();

	 ~FHITerrainVoronoi();
	
private:
	int32 Seed;
	float SizeX;
	float SizeY;
	float NumSites;
	FBox Bounds;
	TArray<FVector> Sites;
	TArray<FVoronoiCellInfo> AllCells;
	FVoronoiDiagram* VoronoiDiagram = nullptr;
};
