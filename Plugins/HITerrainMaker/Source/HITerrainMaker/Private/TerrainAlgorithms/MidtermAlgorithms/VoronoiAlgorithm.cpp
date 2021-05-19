// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainAlgorithms/MidtermAlgorithms/VoronoiAlgorithm.h"

#include "TerrainDatas/HITerrainData.h"

void UVoronoiAlgorithm::Init(FTerrainInformationPtr InInformation)
{
	Super::Init(InInformation);
	int32 Size = Information->ChunkNum * Information->ChunkSize / 100;
	Voronoi.SetSeed(Information->Seed);
	Voronoi.SetAmplitude(1000);
	Voronoi.SetNumSites(200);
}

void UVoronoiAlgorithm::ApplyAlgorithm(UHITerrainData* Data)
{
	Super::ApplyAlgorithm(Data);
	Voronoi.ApplyModule(Data);
}

void UVoronoiAlgorithm::DebugAlgorithm(UHITerrainData* Data)
{
	Super::DebugAlgorithm(Data);
	Voronoi.ApplyModule(Data);
}
