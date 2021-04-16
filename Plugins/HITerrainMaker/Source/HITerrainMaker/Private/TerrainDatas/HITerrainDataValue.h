// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

UENUM()
enum class ETerrainDataType
{
	None = 0,
	FLOAT,
};

class FHITerrainDataValue
{
public:
	FHITerrainDataValue():Type(ETerrainDataType::None){};
	
	virtual float GetNumber();
	virtual bool TryGetNumber(float& OutNumber);

protected:
	ETerrainDataType Type;

	virtual FString GetType() = 0;
	void ErrorMessage(const FString& InType);
};

class FHITerrainFloatValue: public FHITerrainDataValue
{
public:
	FHITerrainFloatValue(float InValue):Value(InValue)
	{
		Type = ETerrainDataType::FLOAT;
	}

	virtual float GetNumber() override
	{
		return Value;
	}

	virtual bool TryGetNumber(float& OutNumber) override
	{
		OutNumber = Value;
		return true;
	}

protected:
	virtual FString GetType() override
	{
		return TEXT("Float");
	}


private:
	float Value;
};