// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainAlgorithms/TestAlgorithms/TestAlgorithm.h"

void UTestAlgorithm::Init(FTerrainInformationPtr InInformation)
{
	Super::Init(InInformation);
	Perlin.SetSeed(Information->Seed);
	Perlin.SetAmplitude(2000);
	Perlin.SetScale(0.01);
	Perlin.SetTargetChannel("height");
}

void UTestAlgorithm::ApplyAlgorithm(UHITerrainData* Data)
{
	Perlin.ApplyModule(Data);
}

void UTestAlgorithm::DebugAlgorithm(UHITerrainData* Data)
{
	Perlin.ApplyModule(Data);
}
