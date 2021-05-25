// Fill out your copyright notice in the Description page of Project Settings.


#include "HITerrainFoliage.h"

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
