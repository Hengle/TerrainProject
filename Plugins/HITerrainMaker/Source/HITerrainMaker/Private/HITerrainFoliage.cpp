﻿#include "HITerrainFoliage.h"

UHITerrainFoliage* UHITerrainFoliage::Instance = nullptr;

UHITerrainFoliage* UHITerrainFoliage::Get()
{
	if (Instance == nullptr)
	{
		Instance = NewObject<UHITerrainFoliage>();
		Instance->AddToRoot();
	}
	return Instance;
}

UStaticMesh* UHITerrainFoliage::GetFoliageOfType(uint8 Type)
{
	if(Type == 1)
	{
		return StaticMeshBush01;
	}
	if(Type == 2)
	{
		return StaticMeshBush02;
	}
	if(Type == 3)
	{
		return StaticMeshBush03;
	}
	if(Type == 4)
	{
		return StaticMeshBush04;
	}
	if(Type == 5)
	{
		return StaticMeshBush05;
	}
	if(Type == 11)
	{
		return StaticMeshTree01;
	}
	if(Type == 12)
	{
		return StaticMeshTree02;
	}
	return StaticMeshBush01;
}
