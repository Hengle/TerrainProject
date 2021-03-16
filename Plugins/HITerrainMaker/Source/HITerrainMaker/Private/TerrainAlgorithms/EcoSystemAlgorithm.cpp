// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainAlgorithms/EcoSystemAlgorithm.h"

#include "TerrainDatas/HITerrainData.h"

void UEcoSystemAlgorithm::Init(FTerrainInformationPtr InInformation)
{
	Super::Init(InInformation);
	Voronoi.SetSeed(Information->Seed);
	Voronoi.SetFrequency(Information->TEST_VORONOI_FREQUENCY);
	Voronoi.SetDisplacement(1.0);
}

void UEcoSystemAlgorithm::Apply(UHITerrainData* Data)
{
	Super::Apply(Data);
}

void UEcoSystemAlgorithm::DebugApply(UHITerrainData* Data)
{
	Super::DebugApply(Data);
	int32 Size = Data->Size();
	for(int32 i = 0; i < Size; i++)
	{
		for(int32 j = 0; j < Size; j++)
		{
			float Value = Voronoi.GetValue(i * 0.01f, j * 0.01f, 0) * 1000;
			Data->SetSample(i, j, Value);
		}
	}
}
