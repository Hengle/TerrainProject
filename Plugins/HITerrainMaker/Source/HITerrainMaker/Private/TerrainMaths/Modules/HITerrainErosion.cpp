// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainMaths/Modules/HITerrainErosion.h"

#include "TerrainMaths/HITerrainMathMisc.h"


FHITerrainErosion::FHITerrainErosion():NumIteration(40), DeltaTime(1.0f / 60), HydroErosionScale(1.0), RainAmount(1000.0),
	EvaporationAmount(0.5), HydroErosionAngle(50), ErosionScale(0.012), DepositionScale(0.012), SedimentCapacityScale(1),
	ThermalErosionScale(1.0)
{
	
}

FHITerrainErosion::~FHITerrainErosion()
{
	
}

void FHITerrainErosion::SetNumIteration(int32 InNumIteration)
{
	NumIteration = InNumIteration;
}

void FHITerrainErosion::SetDeltaTime(float InDeltaTime)
{
	DeltaTime = InDeltaTime;
}

void FHITerrainErosion::SetHydroErosionScale(float InHydroErosionScale)
{
	HydroErosionScale = InHydroErosionScale;
}

void FHITerrainErosion::SetRainAmount(float InRainAmount)
{
	RainAmount = InRainAmount;
}

void FHITerrainErosion::SetEvaporationAmount(float InEvaporationAmount)
{
	EvaporationAmount = InEvaporationAmount;
}

void FHITerrainErosion::SetHydroErosionAngle(float InHydroErosionAngle)
{
	HydroErosionAngle = InHydroErosionAngle;
}

void FHITerrainErosion::SetErosionScale(float InErosionScale)
{
	ErosionScale = InErosionScale;
}

void FHITerrainErosion::SetDepositionScale(float InDepositionScale)
{
	DepositionScale = InDepositionScale;
}

void FHITerrainErosion::SetSedimentCapacityScale(float InSedimentCapacityScale)
{
	SedimentCapacityScale = InSedimentCapacityScale;
}

void FHITerrainErosion::SetThermalErosionScale(float InThermalErosionScale)
{
	ThermalErosionScale = InThermalErosionScale;
}

/*
 * 1、加水
 * 2、水流模拟
 * 3、侵蚀、沉积模拟
 * 4、沉积物重力模拟
 * 5、水蒸发
 */
void FHITerrainErosion::ApplyModule(UHITerrainData* Data)
{
	Data->AddChannel("water", ETerrainDataType::FLOAT);
	Data->AddChannel("sediment", ETerrainDataType::FLOAT);
	Data->AddChannel("flux", ETerrainDataType::FQUAT);
	Data->AddChannel("velocity", ETerrainDataType::FVECTOR);
	HeightChannel = Data->GetChannel("height");
	WaterChannel = Data->GetChannel("water");
	SedimentChannel = Data->GetChannel("sediment");
	VelocityChannel = Data->GetChannel("velocity");
	FluxChannel = Data->GetChannel("flux");
	/*
	 * 四个方向上的流量，顺序是L、R、T、B
	 */
	ApplyInitialization();
	for(int32 Iterate = 0; Iterate < NumIteration; Iterate++)
	{
		UE_LOG(LogHITerrain, Warning, TEXT("Erosion Iteration %d"), Iterate);
		ApplyRainSimulation();
		ApplyFlowSimulation();
		ApplyErosionDepositionSimulation();
		ApplySedimentSimulation();
		ApplyThermalErosionSimulation();
		ApplyEvaporationSimulation();
	}
}

void FHITerrainErosion::ApplyInitialization()
{
	int32 SizeX = HeightChannel->GetSizeX();
	int32 SizeY = HeightChannel->GetSizeY();
	/*
	 * 初始化Flux通道
	 */
	for(int32 i = 0; i < SizeX; i++)
	{
		for(int32 j = 0; j < SizeY; j++)
		{
			// float LValue = i == 0? 0: (HeightChannel->GetValue(i - 1, j)->GetNumber() + WaterChannel->GetValue(i - 1, j)->GetNumber());
			// float RValue = i == SizeX - 1? 0: (HeightChannel->GetValue(i + 1, j)->GetNumber() + WaterChannel->GetValue(i + 1, j)->GetNumber());
			// float TValue = j == 0? 0: (HeightChannel->GetValue(i, j - 1)->GetNumber() + WaterChannel->GetValue(i, j - 1)->GetNumber());
			// float BValue = j == SizeY - 1? 0: (HeightChannel->GetValue(i, j + 1)->GetNumber() + WaterChannel->GetValue(i, j + 1)->GetNumber());
			// float Value = HeightChannel->GetValue(i, j)->GetNumber() + WaterChannel->GetValue(i, j)->GetNumber();
			FQuat FluxValue(0.0f, 0.0f, 0.0f, 0.0f);
			// FluxValue.X = FMath::Max(0.0f, Value - LValue);
			// FluxValue.Y = FMath::Max(0.0f, Value - RValue);
			// FluxValue.Z = FMath::Max(0.0f, Value - TValue);
			// FluxValue.W = FMath::Max(0.0f, Value - BValue);
			// FluxValue /= (FluxValue.X + FluxValue.Y + FluxValue.Z + FluxValue.W);
			FluxChannel->SetFQuat(i, j, FluxValue);
		}
	}
}

/*
 * 对每个格子加恒定的水量
 */
void FHITerrainErosion::ApplyRainSimulation()
{
	int32 SizeX = HeightChannel->GetSizeX();
	int32 SizeY = HeightChannel->GetSizeY();
	for(int32 i = 0; i < SizeX; i++)
	{
		for(int32 j = 0; j < SizeY; j++)
		{
			WaterChannel->SetFloat(i, j, WaterChannel->GetFloat(i, j) + RainAmount * DeltaTime);
		}
	}
}

/*
 * 模拟水流，计算每个格子的流量，更新水通道
 */
void FHITerrainErosion::ApplyFlowSimulation()
{
	int32 SizeX = HeightChannel->GetSizeX();
	int32 SizeY = HeightChannel->GetSizeY();
	/*
	 * 算流量
	 */
	for(int32 i = 0; i < SizeX; i++)
	{
		for(int32 j = 0; j < SizeY; j++)
		{
			float LValue = i == 0? 0: (HeightChannel->GetFloat(i - 1, j) + WaterChannel->GetFloat(i - 1, j));
			float RValue = i == SizeX - 1? 0: (HeightChannel->GetFloat(i + 1, j) + WaterChannel->GetFloat(i + 1, j));
			float TValue = j == 0? 0: (HeightChannel->GetFloat(i, j - 1) + WaterChannel->GetFloat(i, j - 1));
			float BValue = j == SizeY - 1? 0: (HeightChannel->GetFloat(i, j + 1) + WaterChannel->GetFloat(i, j + 1));
			float Value = HeightChannel->GetFloat(i, j) + WaterChannel->GetFloat(i, j);
			FQuat FluxValue;
			FluxValue.X = FMath::Max(0.0f, Value - LValue + FluxChannel->GetFQuat(i, j).X * DeltaTime);
			FluxValue.Y = FMath::Max(0.0f, Value - RValue + FluxChannel->GetFQuat(i, j).Y * DeltaTime);
			FluxValue.Z = FMath::Max(0.0f, Value - TValue + FluxChannel->GetFQuat(i, j).Z * DeltaTime);
			FluxValue.W = FMath::Max(0.0f, Value - BValue + FluxChannel->GetFQuat(i, j).W * DeltaTime);
			float K = FMath::Max(1.0f, WaterChannel->GetFloat(i, j) / ((FluxValue.X + FluxValue.Y + FluxValue.Z + FluxValue.W) * DeltaTime));
			FluxValue /= K;
			FluxChannel->SetFQuat(i, j, FluxValue);
		}
	}
	/*
	 * 根据流量算之后的水量
	 */
	for(int32 i = 0; i < SizeX; i++)
	{
		for(int32 j = 0; j < SizeY; j++)
		{
			FQuat FluxValue = FluxChannel->GetFQuat(i, j);
			float OutValue = FluxValue.X + FluxValue.Y + FluxValue.Z + FluxValue.W;
			float LInValue = i == 0? 0: FluxChannel->GetFQuat(i - 1, j).Y;
			float RInValue = i == SizeX - 1? 0: FluxChannel->GetFQuat(i + 1, j).X;
			float TInValue = j == 0? 0: FluxChannel->GetFQuat(i, j - 1).W;
			float BInValue = j == SizeY - 1? 0: FluxChannel->GetFQuat(i, j + 1).Z;
			float DeltaWaterValue = LInValue + RInValue + TInValue + BInValue - OutValue;
			WaterChannel->SetFloat(i, j, WaterChannel->GetFloat(i, j) + DeltaWaterValue);
		}
	}
}

/*
 * 根据流量计算动量，根据动量等信息算侵蚀、沉积
 */
void FHITerrainErosion::ApplyErosionDepositionSimulation()
{
	int32 SizeX = HeightChannel->GetSizeX();
	int32 SizeY = HeightChannel->GetSizeY();
	/*
	* 根据流量算动量，再算侵蚀、沉积
	*/
	for(int32 i = 0; i < SizeX; i++)
	{
		for(int32 j = 0; j < SizeY; j++)
		{
			FQuat FluxValue = FluxChannel->GetFQuat(i, j);
			float LInValue = i == 0? 0: FluxChannel->GetFQuat(i - 1, j).Y;
			float RInValue = i == SizeX - 1? 0: FluxChannel->GetFQuat(i + 1, j).X;
			float TInValue = j == 0? 0: FluxChannel->GetFQuat(i, j - 1).W;
			float BInValue = j == SizeY - 1? 0: FluxChannel->GetFQuat(i, j + 1).Z;
			FVector Velocity;
			Velocity.X = (LInValue - FluxValue.X + FluxValue.Y - RInValue) / 2;
			Velocity.Y = (TInValue - FluxValue.Z + FluxValue.W - BInValue) / 2;
			VelocityChannel->SetFVector(i, j, Velocity);
			float VelocityValue = FMath::Sqrt(Velocity.X * Velocity.X + Velocity.Y * Velocity.Y);
			float SedimentCapacity = FMath::Sin(HydroErosionAngle) * VelocityValue;
			float SedimentValue = SedimentChannel->GetFloat(i, j);
			float Sediment = SedimentChannel->GetFloat(i, j);
			float Height = HeightChannel->GetFloat(i, j) - Sediment;
			if(SedimentValue > SedimentCapacity)
			{
				Height += DepositionScale * (SedimentValue - SedimentCapacity);
				Sediment -= DepositionScale * (SedimentValue - SedimentCapacity);
			}
			else
			{
				Height -= ErosionScale * (SedimentCapacity - SedimentValue);
				Sediment += ErosionScale * (SedimentCapacity - SedimentValue);
			}
			HeightChannel->SetFloat(i, j, Height + Sediment);
			SedimentChannel->SetFloat(i, j, Sediment);
		}
	}
	
}

/*
 * 算沉积物的转移
 */
void FHITerrainErosion::ApplySedimentSimulation()
{
	int32 SizeX = HeightChannel->GetSizeX();
	int32 SizeY = HeightChannel->GetSizeY();
	/*
	* 沉积物的转移
	*/
	for(int32 i = 0; i < SizeX; i++)
	{
		for(int32 j = 0; j < SizeY; j++)
		{
			FVector Velocity = VelocityChannel->GetFVector(i, j);
			float FromPosX = i - Velocity.X * DeltaTime;
			float FromPosY = j - Velocity.Y * DeltaTime;
			int32 X0 = FMath::FloorToInt(FromPosX);
			int32 Y0 = FMath::FloorToInt(FromPosY);
			int32 X1 = X0 + 1;
			int32 Y1 = Y0 + 1;
			X0 = FMath::Clamp(X0, 0, SizeX - 1);
			Y0 = FMath::Clamp(Y0, 0, SizeY - 1);
			X1 = FMath::Clamp(X1, 0, SizeX - 1);
			Y1 = FMath::Clamp(Y1, 0, SizeY - 1);
			float S00 = SedimentChannel->GetFloat(X0, Y0);
			float S01 = SedimentChannel->GetFloat(X0, Y1);
			float S10 = SedimentChannel->GetFloat(X1, Y0);
			float S11 = SedimentChannel->GetFloat(X1, Y1);
			int32 AlphaX = FromPosX - FMath::FloorToInt(FromPosX);
			int32 AlphaY = FromPosY - FMath::FloorToInt(FromPosY);
			float NewSediment = UHITerrainMathMisc::LinearLerp2D(S00, S01, S10, S11, AlphaX, AlphaY);
			SedimentChannel->SetFloat(i, j, NewSediment);
		}
	}
}

void FHITerrainErosion::ApplyThermalErosionSimulation()
{
}

/*
 * 水蒸发
 */
void FHITerrainErosion::ApplyEvaporationSimulation()
{
	int32 SizeX = HeightChannel->GetSizeX();
	int32 SizeY = HeightChannel->GetSizeY();
	for(int32 i = 0; i < SizeX; i++)
	{
		for(int32 j = 0; j < SizeY; j++)
		{
			float WaterValue = WaterChannel->GetFloat(i, j);
			WaterValue *= (1 - EvaporationAmount * DeltaTime);
			WaterChannel->SetFloat(i, j, WaterValue);
		}
	}
}
