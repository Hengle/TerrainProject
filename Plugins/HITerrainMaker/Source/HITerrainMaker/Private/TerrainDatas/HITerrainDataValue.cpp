// Fill out your copyright notice in the Description page of Project Settings.


#include "HITerrainDataValue.h"

#include "HITerrainCommons.h"

float FHITerrainDataValue::GetNumber()
{
	float Number = 0.0f;
	if(!TryGetNumber(Number))
	{
		ErrorMessage(TEXT("Number"));
	}
	return Number;
}

bool FHITerrainDataValue::TryGetNumber(float& OutNumber)
{
	return false;
}

void FHITerrainDataValue::ErrorMessage(const FString& InType)
{
	UE_LOG(LogHITerrain, Error, TEXT("Terrain Data Value of type '%s' used as a '%s'."), *GetType(), *InType);
}
