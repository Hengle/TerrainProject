#include "TerrainAlgorithms/TestAlgorithms/TestAlgorithm.h"

void UTestAlgorithm::Init(FTerrainInformationPtr InInformation)
{
	Super::Init(InInformation);
	Perlin.SetSeed(Information->Seed);
	Perlin.SetAmplitude(2000);
	Perlin.SetScale(0.005);
	Perlin.SetTargetChannel("height");

	Erosion.SetSeed(Information->Seed);
}

void UTestAlgorithm::ApplyAlgorithm(UHITerrainData* Data)
{
	Super::ApplyAlgorithm(Data);
	Data->AddChannel("r", ETerrainDataType::FLOAT);
	Data->AddChannel("g", ETerrainDataType::FLOAT);
	Data->AddChannel("b", ETerrainDataType::FLOAT);
	Data->AddChannel("a", ETerrainDataType::FLOAT);
	Perlin.ApplyModule(Data);
	Erosion.ApplyModule(Data);
}

void UTestAlgorithm::DebugAlgorithm(UHITerrainData* Data)
{
	Super::ApplyAlgorithm(Data);
	Data->AddChannel("r", ETerrainDataType::FLOAT);
	Data->AddChannel("g", ETerrainDataType::FLOAT);
	Data->AddChannel("b", ETerrainDataType::FLOAT);
	Data->AddChannel("a", ETerrainDataType::FLOAT);
	Perlin.ApplyModule(Data);
	Erosion.ApplyModule(Data);
}
