// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TerrainMaths/2DArray.h"
#include "TerrainMaths/HITerrainMathMisc.h"

UENUM()
enum class ETerrainDataType: uint8
{
	None = 0,
	BOOL,
	FLOAT,
	FVECTOR,
	FQUAT,
};

class FHITerrainChannel
{
public:
	static TSharedPtr<FHITerrainChannel> CreateChannelByType(FString InChannelName, int32 InSizeX, int32 InSizeY, ETerrainDataType InType);
	
	static TSharedPtr<FHITerrainChannel> CreateFloatChannel(FString InChannelName, int32 InSizeX, int32 InSizeY);
	static TSharedPtr<FHITerrainChannel> CreateBoolChannel(FString InChannelName, int32 InSizeX, int32 InSizeY);
	static TSharedPtr<FHITerrainChannel> CreateFVectorChannel(FString InChannelName, int32 InSizeX, int32 InSizeY);
	static TSharedPtr<FHITerrainChannel> CreateFQuatChannel(FString InChannelName, int32 InSizeX, int32 InSizeY);
	
	static TSharedPtr<FHITerrainChannel> CopyChannel(FString InChannelName, TSharedPtr<FHITerrainChannel> FromChannel);

public:
	virtual float GetFloat(int32 X, int32 Y);
	virtual bool TryGetFloat(int32 X, int32 Y, float& OutFloat);
	virtual void SetFloat(int32 X, int32 Y, const float& InFloat);

	virtual FVector GetFVector(int32 X, int32 Y);
	virtual bool TryGetFVector(int32 X, int32 Y, FVector& OutFVector);
	virtual void SetFVector(int32 X, int32 Y, const FVector& InFVector);

	virtual bool GetBool(int32 X, int32 Y);
	virtual bool TryGetBool(int32 X, int32 Y, bool& OutBool);
	virtual void SetBool(int32 X, int32 Y, const bool InBool);

	virtual FQuat GetFQuat(int32 X, int32 Y);
	virtual bool TryGetFQuat(int32 X, int32 Y, FQuat& OutFQuat);
	virtual void SetFQuat(int32 X, int32 Y, const FQuat& InFQuat);

	virtual FString GetTypeName() = 0;
	virtual ETerrainDataType GetType() = 0;

	int32 GetSizeX();

	int32 GetSizeY();

	void ErrorMessage(const FString& InType);

public:
	virtual ~FHITerrainChannel(){}

protected:
	int32 SizeX;
	int32 SizeY;
	ETerrainDataType Type;
	FString ChannelName;
};

class FHITerrainFloatChannel: public FHITerrainChannel
{
public:
	FHITerrainFloatChannel(int32 InSizeX, int32 InSizeY)
	{
		Data = TFixed2DArray<float>(InSizeX, InSizeY, 0.0f);
	}
	
	virtual FString GetTypeName() override
	{
		return "Float";
	}

	virtual ETerrainDataType GetType() override
	{
		return ETerrainDataType::FLOAT;
	}
	
	virtual float GetFloat(int32 X, int32 Y) override
	{
		return Data.GetValue(X, Y);	
	}
	
	virtual bool TryGetFloat(int32 X, int32 Y, float& OutFloat) override
	{
		OutFloat = Data.GetValue(X, Y);
		return true;
	}
	
	virtual void SetFloat(int32 X, int32 Y, const float& InFloat) override
	{
		if(FHITerrainMathMisc::IsNaN(InFloat))
		{
			UE_LOG(LogHITerrain, Warning, TEXT("[%s]::SetFloat NaN"), *ChannelName)
		}
		Data.SetValue(X, Y, InFloat);
	}

	virtual const TArray<float>& GetRawFloatArray()
	{
		return Data.RawArray;
	};

private:
	TFixed2DArray<float> Data;
};

class FHITerrainBoolChannel: public FHITerrainChannel
{
public:
	FHITerrainBoolChannel(int32 InSizeX, int32 InSizeY)
	{
		Data = TFixed2DArray<bool>(InSizeX, InSizeY, false);
	}
	
	virtual FString GetTypeName() override
	{
		return "Bool";
	}

	virtual ETerrainDataType GetType() override
	{
		return ETerrainDataType::BOOL;
	}
	
	virtual bool GetBool(int32 X, int32 Y) override
	{
		return Data.GetValue(X, Y);	
	}
	
	virtual bool TryGetBool(int32 X, int32 Y, bool& OutBool) override
	{
		OutBool = Data.GetValue(X, Y);
		return true;
	}
	
	virtual void SetBool(int32 X, int32 Y, const bool InBool) override
	{
		Data.SetValue(X, Y, InBool);
	}

	virtual const TArray<bool>& GetRawBoolArray()
	{
		return Data.RawArray;
	};

private:
	TFixed2DArray<bool> Data;
};

class FHITerrainFVectorChannel: public FHITerrainChannel
{
public:
	FHITerrainFVectorChannel(int32 InSizeX, int32 InSizeY)
	{
		Data = TFixed2DArray<FVector>(InSizeX, InSizeY, FVector(0.0f, 0.0f, 0.0f));
	}
	
	virtual FString GetTypeName() override
	{
		return "FVector";
	}
	
	virtual ETerrainDataType GetType() override
	{
		return ETerrainDataType::FVECTOR;
	}

	virtual FVector GetFVector(int32 X, int32 Y) override
	{
		return Data.GetValue(X, Y);	
	}
	
	virtual bool TryGetFVector(int32 X, int32 Y, FVector& OutFVector) override
	{
		OutFVector = Data.GetValue(X, Y);
		return true;
	}
	
	virtual void SetFVector(int32 X, int32 Y, const FVector& InFVector) override
	{
		if(FHITerrainMathMisc::IsNaN(InFVector))
		{
			UE_LOG(LogHITerrain, Warning, TEXT("[%s]::SetFVector NaN"), *ChannelName)
		}
		Data.SetValue(X, Y, InFVector);
	}
	
	virtual const TArray<FVector>& GetRawFVectorArray()
	{
		return Data.RawArray;
	};

private:
	TFixed2DArray<FVector> Data;
};

class FHITerrainFQuatChannel: public FHITerrainChannel
{
public:
	FHITerrainFQuatChannel(int32 InSizeX, int32 InSizeY)
	{
		Data = TFixed2DArray<FQuat>(InSizeX, InSizeY, FQuat(0.0f, 0.0f, 0.0f, 0.0f));
	}
	
	virtual FString GetTypeName() override
	{
		return "FQuat";
	}

	virtual ETerrainDataType GetType() override
	{
		return ETerrainDataType::FQUAT;
	}

	virtual FQuat GetFQuat(int32 X, int32 Y) override
	{
		return Data.GetValue(X, Y);	
	}
	
	virtual bool TryGetFQuat(int32 X, int32 Y, FQuat& OutFQuat) override
	{
		OutFQuat = Data.GetValue(X, Y);
		return true;
	}
	
	virtual void SetFQuat(int32 X, int32 Y, const FQuat& InFQuat) override
	{
		if(FHITerrainMathMisc::IsNaN(InFQuat))
		{
			UE_LOG(LogHITerrain, Warning, TEXT("[%s]::SetFQuat NaN"), *ChannelName)
		}
		Data.SetValue(X, Y, InFQuat);
	}

	virtual const TArray<FQuat>& GetRawFQuatArray()
	{
		return Data.RawArray;
	};

private:
	TFixed2DArray<FQuat> Data;
};