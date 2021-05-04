// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainAlgorithms/TestAlgorithms/TestAlgorithm2.h"

void UTestAlgorithm2::Init(FTerrainInformationPtr InInformation)
{
	Super::Init(InInformation);
	Perlin.SetSeed(Information->Seed);
	Perlin.SetAmplitude(2000);
	Perlin.SetScale(0.005);
	Perlin.SetTargetChannel("height");

	Erosion.SetSeed(Information->Seed);
	Erosion.SetEnableHydroErosion(false);
	Erosion.SetThermalErosionScale(10.0f);
}

void UTestAlgorithm2::ApplyAlgorithm(UHITerrainData* Data)
{
	Super::ApplyAlgorithm(Data);
	// Perlin.ApplyModule(Data);
	// Erosion.ApplyModule(Data);
	ErosionGPU.ApplyModule(Data);
}

void UTestAlgorithm2::DebugAlgorithm(UHITerrainData* Data)
{
	Super::ApplyAlgorithm(Data);
	// Perlin.ApplyModule(Data);
	// Erosion.ApplyModule(Data);
	ErosionGPU.ApplyModule(Data);
}