// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainAlgorithms/TestAlgorithms/TestAlgorithm3.h"

void UTestAlgorithm3::Init(FTerrainInformationPtr InInformation)
{
	Super::Init(InInformation);
	Perlin.SetSeed(Information->Seed + 1);
	Perlin.SetAmplitude(1.0f);
	Perlin.SetScale(0.01);
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
	int32 Size = Data->Size();
	for(int32 i = 0; i < Size; i++)
	{
		for(int32 j = 0; j < Size; j++)
		{
			float Height = Data->GetHeightValue(i, j);
			if(Height < 0.5)
			{
				Data->SetHeightValue(i, j, 1000.0f);
			}
			else
			{
				Data->SetHeightValue(i, j, 0.0f);
			}
		}
	}
	SlopeGPU.ApplyModule(Data);
}
