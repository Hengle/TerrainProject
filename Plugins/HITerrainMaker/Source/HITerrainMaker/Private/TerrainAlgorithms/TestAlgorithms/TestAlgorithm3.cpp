#include "TerrainAlgorithms/TestAlgorithms/TestAlgorithm3.h"

void UTestAlgorithm3::Init(FTerrainInformationPtr InInformation)
{
	Super::Init(InInformation);
	Perlin.SetSeed(Information->Seed);
	Perlin.SetAmplitude(Information->Terrain_Amplitude);
	Perlin.SetScale(Information->Terrain_Scale);
	Perlin.SetTargetChannel("height");
}

void UTestAlgorithm3::ApplyAlgorithm(UHITerrainData* Data)
{
	Super::ApplyAlgorithm(Data);
}

void UTestAlgorithm3::DebugAlgorithm(UHITerrainData* Data)
{
	Super::DebugAlgorithm(Data);
	Data->AddChannel("r", ETerrainDataType::FLOAT);
	Data->AddChannel("g", ETerrainDataType::FLOAT);
	Data->AddChannel("b", ETerrainDataType::FLOAT);
	Data->AddChannel("a", ETerrainDataType::FLOAT);
	Perlin.ApplyModule(Data);
}
