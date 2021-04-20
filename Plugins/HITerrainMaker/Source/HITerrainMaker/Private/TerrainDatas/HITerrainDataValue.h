// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

UENUM()
enum class ETerrainDataType: uint8
{
	None = 0,
	BOOL,
	FLOAT,
	FVECTOR,
	FQUAT,
};

class FHITerrainDataValue
{
public:
	FHITerrainDataValue():Type(ETerrainDataType::None){};
	
	virtual float GetNumber();
	virtual bool TryGetNumber(float& OutNumber);
	virtual void SetNumber(const float& InNumber);

	virtual FVector GetFVector();
	virtual bool TryGetFVector(FVector& OutFVector);
	virtual void SetFVector(const FVector& InFVector);

	virtual bool GetBool();
	virtual bool TryGetBool(bool& OutBool);
	virtual void SetBool(const bool InBool);

	virtual FQuat GetFQuat();
	virtual bool TryGetFQuat(FQuat& OutFQuat);
	virtual void SetFQuat(const FQuat& InFQuat);
	
	void CopyFromValue(TSharedPtr<FHITerrainDataValue> Value);
	
	virtual ~FHITerrainDataValue(){};

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

	virtual void SetNumber(const float& InNumber) override
	{
		Value = InNumber;		
	}

protected:
	virtual FString GetType() override
	{
		return TEXT("Float");
	}


private:
	float Value;
};

class FHITerrainFVectorValue: public FHITerrainDataValue
{
public:
	FHITerrainFVectorValue(const FVector& InValue):Value(InValue)
	{
		Type = ETerrainDataType::FVECTOR;
	}
	
	virtual FVector GetFVector() override
	{
		return Value;
	}
	
	virtual bool TryGetFVector(FVector& OutFVector) override
	{
		OutFVector = Value;
		return true;
	}

	virtual void SetFVector(const FVector& InFVector) override
	{
		Value = InFVector;		
	}

protected:
	virtual FString GetType() override
	{
		return TEXT("FVector");
	}

private:
	FVector Value;
};

class FHITerrainBoolValue: public FHITerrainDataValue
{
public:
	FHITerrainBoolValue(bool InValue):Value(InValue)
	{
		Type = ETerrainDataType::BOOL;
	}
	
	virtual bool GetBool() override
	{
		return Value;
	}
	
	virtual bool TryGetBool(bool& OutBool) override
	{
		OutBool = Value;
		return true;
	}
	
	virtual void SetBool(const bool InBool) override
	{
		Value = InBool;
	}

protected:
	virtual FString GetType() override
	{
		return TEXT("Bool");
	}

private:
	bool Value;
};

class FHITerrainFQuatValue: public FHITerrainDataValue
{
public:
	FHITerrainFQuatValue(const FQuat& InValue):Value(InValue)
	{
		Type = ETerrainDataType::FQUAT;
	}
	
	virtual FQuat GetFQuat() override
	{
		return Value;
	}
	
	virtual bool TryGetFQuat(FQuat& OutFQuat) override
	{
		OutFQuat = Value;
		return true;
	}

	virtual void SetFQuat(const FQuat& InFQuat) override
	{
		Value = InFQuat;		
	}

protected:
	virtual FString GetType() override
	{
		return TEXT("FQuat");
	}

private:
	FQuat Value;
};