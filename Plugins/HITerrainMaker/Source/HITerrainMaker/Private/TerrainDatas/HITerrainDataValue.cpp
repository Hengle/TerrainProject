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

void FHITerrainDataValue::SetNumber(const float& InNumber)
{
}

void FHITerrainDataValue::SetFVector(const FVector& InFVector)
{
}

void FHITerrainDataValue::SetBool(const bool InBool)
{
}

FQuat FHITerrainDataValue::GetFQuat()
{
	FQuat Value = FQuat();
	if(!TryGetFQuat(Value))
	{
		ErrorMessage(TEXT("FQuat"));
	}
	return Value;
}

bool FHITerrainDataValue::TryGetFQuat(FQuat& OutFQuat)
{
	return false;
}

void FHITerrainDataValue::SetFQuat(const FQuat& InFQuat)
{
}

bool FHITerrainDataValue::GetBool()
{
	bool Value = false;
	if(!TryGetBool(Value))
	{
		ErrorMessage(TEXT("Bool"));
	}
	return Value;
}

bool FHITerrainDataValue::TryGetBool(bool& OutBool)
{
	return false;
}

void FHITerrainDataValue::CopyFromValue(TSharedPtr<FHITerrainDataValue> Value)
{
	if(Type == ETerrainDataType::FLOAT)
	{
		SetNumber(Value->GetNumber());
	}
	else if(Type == ETerrainDataType::FVECTOR)
	{
		SetFVector(Value->GetFVector());
	}
	else if(Type == ETerrainDataType::BOOL)
	{
		SetBool(Value->GetBool());
	}
}

FVector FHITerrainDataValue::GetFVector()
{
	FVector Vector = FVector();
	if(!TryGetFVector(Vector))
	{
		ErrorMessage(TEXT("FVector"));
	}
	return Vector;
}

bool FHITerrainDataValue::TryGetFVector(FVector& OutFVector)
{
	return false;
}

void FHITerrainDataValue::ErrorMessage(const FString& InType)
{
	UE_LOG(LogHITerrain, Error, TEXT("Terrain Data Value of type '%s' used as a '%s'."), *GetType(), *InType);
}
