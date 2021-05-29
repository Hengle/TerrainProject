// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainAlgorithms/TestAlgorithms/TestAlgorithm2.h"

void UTestAlgorithm2::Init(FTerrainInformationPtr InInformation)
{
	Super::Init(InInformation);
	Perlin.SetSeed(Information->Seed);
	Perlin.SetAmplitude(4000);
	Perlin.SetScale(0.002);
	Perlin.SetTargetChannel("height");
	ErosionGPU.SetNumIteration(Information->Erosion_IterationNum);
	ErosionGPU.SetDeltaTime(Information->Erosion_DeltaTime);
	ErosionGPU.SetEnableHydroErosion(Information->Erosion_EnableHydroErosion);
	ErosionGPU.SetEnableThermalErosion(Information->Erosion_EnableThermalErosion);
	ErosionGPU.SetRainAmount(Information->Erosion_RainScale);
	ErosionGPU.SetEvaporationAmount(Information->Erosion_EvaporationScale);
	ErosionGPU.SetErosionScale(Information->Erosion_HydroErosionScale);
	ErosionGPU.SetDepositionScale(Information->Erosion_HydroDepositionScale);
	ErosionGPU.SetThermalErosionScale(Information->Erosion_ThermalErosionScale);
}

void UTestAlgorithm2::ApplyAlgorithm(UHITerrainData* Data)
{
	Super::ApplyAlgorithm(Data);
	Perlin.ApplyModule(Data);
	ErosionGPU.ApplyModule(Data);
}

void UTestAlgorithm2::DebugAlgorithm(UHITerrainData* Data)
{
	Super::ApplyAlgorithm(Data);
	Data->AddChannel("sediment", ETerrainDataType::FLOAT);
	Data->AddChannel("water", ETerrainDataType::FLOAT);
	Perlin.ApplyModule(Data);
	ErosionGPU.ApplyModule(Data);
	// WaterFlattenGPU.ApplyModule(Data);
	// ThreadSafeTest.ApplyModule(Data);
}