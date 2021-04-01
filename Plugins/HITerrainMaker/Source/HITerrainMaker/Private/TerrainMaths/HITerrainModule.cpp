// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainMaths/HITerrainModule.h"

void FHITerrainModule::SetSourceModule(FHITerrainModule* SourceModule)
{
	if(SourceModules.Num() >= GetSourceModuleCapacity())
	{
		UE_LOG(LogHITerrain, Error, TEXT("FHITerrainModule::SetSourceModule Num > Capacity!"))
	}
	else
	{
		SourceModules.Add(SourceModule);
	}
}

float FHITerrainModule::GetValue(float X, float Y)
{
	UE_LOG(LogHITerrain, Warning, TEXT("FHITerrainModule::GetValue"))
	return 0.0f;
}

int32 FHITerrainModule::GetSourceModuleCapacity()
{
	UE_LOG(LogHITerrain, Warning, TEXT("FHITerrainModule::GetSourceModuleCapacity"))
	return 0;
}

FHITerrainModule::FHITerrainModule()
{
	
}

FHITerrainModule::~FHITerrainModule()
{
	
}
