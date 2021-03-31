// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainAlgorithms/BasicAlgorithms/BasicAlgorithm.h"

#include <destructible/ExplicitHierarchicalMesh.h>

#include "TerrainDatas/HITerrainData.h"

void UBasicAlgorithm::Init(FTerrainInformationPtr InInformation)
{
	Super::Init(InInformation);
	Landscape.Init(Information->Seed, Information->BG_LandscapeFrequency,
		Information->BG_LandscapeLacunarity, Information->BG_LandscapeOctaveCount);
	// Landscape.AddControlPoint(-1.0f, -1.0f);
	// Landscape.AddControlPoint(-0.5f, -0.8f);
	// Landscape.AddControlPoint(-0.25f, 0.0f);
	// Landscape.AddControlPoint(0.6f, 0.25f);
	// Landscape.AddControlPoint(0.7f, 0.5f);
	// Landscape.AddControlPoint(0.8f, 0.75f);
	// Landscape.AddControlPoint(0.9f, 1.25f);
	// Landscape.AddControlPoint(1.0f, 2.25f);
}

void UBasicAlgorithm::ApplyAlgorithm(UHITerrainData* Data)
{
	Super::ApplyAlgorithm(Data);
    int32 Size = Data->Size();
    for(int32 i = 0; i < Size; i++)
    {
    	for(int32 j = 0; j < Size; j++)
    	{
    		float Value = 0.0f;
    		Value = Landscape.GetValue(i * 0.01f, j * 0.01f) * 2000;
    		Data->SetSampleValue(i, j, Value);
    		if(Value < Information->BG_SeaLevel)
    		{
    			Data->SetSampleType(i, j, ESampleType::OCEAN);
    		}
    		else if(Value < Information->BG_SandLevel)
    		{
    			Data->SetSampleType(i, j, ESampleType::BEACH);
    		}
    		else if(Value < Information->BG_GrassLevel)
    		{
    			Data->SetSampleType(i, j, ESampleType::GRASS);
    		}
    		else if(Value < Information->BG_MountainLevel)
    		{
    			Data->SetSampleType(i, j, ESampleType::MOUNTAIN);
    		}
    		else
    		{
    			Data->SetSampleType(i, j, ESampleType::SNOWY);
    		}
    	}
    }
}

void UBasicAlgorithm::DebugAlgorithm(UHITerrainData* Data)
{
	Super::DebugAlgorithm(Data);
	int32 Size = Data->Size();
	for(int32 i = 0; i < Size; i++)
	{
		for(int32 j = 0; j < Size; j++)
		{
			float Value = 0.0f;
			Value = Landscape.GetValue(i * 0.01f, j * 0.01f) * 2000;
			Data->SetSampleValue(i, j, Value);
		}
	}
}
