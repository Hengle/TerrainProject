#include "TerrainAlgorithms/FinalAlgorithm.h"

void UFinalAlgorithm::Init(FTerrainInformationPtr InInformation)
{
	Super::Init(InInformation);
	Perlin.SetSeed(Information->Seed);
	Perlin.SetAmplitude(Information->Terrain_Amplitude);
	Perlin.SetScale(Information->Terrain_Scale);
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

void UFinalAlgorithm::ApplyAlgorithm(UHITerrainData* Data)
{
	Super::ApplyAlgorithm(Data);
	Data->AddChannel("sediment", ETerrainDataType::FLOAT);
	Data->AddChannel("water", ETerrainDataType::FLOAT);
	Perlin.ApplyModule(Data);
	ErosionGPU.ApplyModule(Data);
}

void UFinalAlgorithm::DebugAlgorithm(UHITerrainData* Data)
{
	Super::ApplyAlgorithm(Data);
	Data->AddChannel("sediment", ETerrainDataType::FLOAT);
	Data->AddChannel("water", ETerrainDataType::FLOAT);
	FDateTime Time1 = FDateTime::Now();
	Perlin.ApplyModule(Data);
	FDateTime Time2 = FDateTime::Now();
	ErosionGPU.ApplyModule(Data);
	LOCK
	UNLOCK
	FDateTime Time3 = FDateTime::Now();
	UE_LOG(LogHITerrain, Warning, TEXT("Perlin: %s"), *(Time2 - Time1).ToString())
	UE_LOG(LogHITerrain, Warning, TEXT("Erosion: %s"), *(Time3 - Time2).ToString())
}