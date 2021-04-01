// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainAlgorithms/BasicAlgorithms/BasicAlgorithm.h"

#include <destructible/ExplicitHierarchicalMesh.h>

#include "TerrainDatas/HITerrainData.h"

void UBasicAlgorithm::Init(FTerrainInformationPtr InInformation)
{
	Super::Init(InInformation);
	Landscape.Init(Information->Seed, Information->BG_LandscapeFrequency,
		Information->BG_LandscapeLacunarity, Information->BG_LandscapeOctaveCount);
	Landscape.AddControlPoint(-1.0f, -1.375f);
	Landscape.AddControlPoint(0.0f, -0.375f);
	Landscape.AddControlPoint(0.0625f, 0.125f);
	Landscape.AddControlPoint(0.125f, 0.25f);
	Landscape.AddControlPoint(0.25f, 1.0f);
	Landscape.AddControlPoint(0.5f, 0.25f);
	Landscape.AddControlPoint(0.75f, 0.25f);
	Landscape.AddControlPoint(1.00f, 0.5f);
	/*
	*     baseContinentDef_cu.AddControlPoint (-2.0000 + SEA_LEVEL,-1.625 + SEA_LEVEL);
	baseContinentDef_cu.AddControlPoint (-1.0000 + SEA_LEVEL,-1.375 + SEA_LEVEL);
	baseContinentDef_cu.AddControlPoint ( 0.0000 + SEA_LEVEL,-0.375 + SEA_LEVEL);
	baseContinentDef_cu.AddControlPoint ( 0.0625 + SEA_LEVEL, 0.125 + SEA_LEVEL);
	baseContinentDef_cu.AddControlPoint ( 0.1250 + SEA_LEVEL, 0.250 + SEA_LEVEL);
	baseContinentDef_cu.AddControlPoint ( 0.2500 + SEA_LEVEL, 1.000 + SEA_LEVEL);
	baseContinentDef_cu.AddControlPoint ( 0.5000 + SEA_LEVEL, 0.250 + SEA_LEVEL);
	baseContinentDef_cu.AddControlPoint ( 0.7500 + SEA_LEVEL, 0.250 + SEA_LEVEL);
	baseContinentDef_cu.AddControlPoint ( 1.0000 + SEA_LEVEL, 0.500 + SEA_LEVEL);
	baseContinentDef_cu.AddControlPoint ( 2.0000 + SEA_LEVEL, 0.500 + SEA_LEVEL);
	 */
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
