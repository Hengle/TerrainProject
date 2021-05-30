#pragma once

#include "CoreMinimal.h"

#include "HITerrainModule.h"
#include "Runtime/Experimental/Voronoi/Public/Voronoi/Voronoi.h"
#include "TerrainMaths/2DArray.h"

class HITERRAINMAKER_API FHITerrainVoronoi: public FHITerrainModule
{
public:
	void SetSeed(int32 InSeed);
	void SetNumSites(int32 InNumSites);
	void SetAmplitude(float InAmplitude);
	void SetSizeX(int32 InSizeX);
	void SetSizeY(int32 InSizeY);

	virtual void ApplyModule(UHITerrainData* Data) override;
	 
	float GetCellValue(float X, float Y);

	const TArray<FVoronoiCellInfo>& GetAllCells();

	int32 Position2Cell(float X, float Y);

	int32 Position2Cell(FVector Position);

	const TArray<FVector>& GetSites();

	 ~FHITerrainVoronoi();
	
private:
	int32 Seed;
	float NumSites;
	float Amplitude;
	int32 SizeX;
	int32 SizeY;
	FBox Bounds;
	TArray<FVector> Sites;
	TArray<FVoronoiCellInfo> AllCells;
	FVoronoiDiagram* VoronoiDiagram = nullptr;
};
