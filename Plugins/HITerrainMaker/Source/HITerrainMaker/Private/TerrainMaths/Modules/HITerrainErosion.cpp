// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainMaths/Modules/HITerrainErosion.h"

#include "TerrainMaths/HITerrainMathMisc.h"


FHITerrainErosion::FHITerrainErosion()
{
	
}

FHITerrainErosion::~FHITerrainErosion()
{
	
}

void FHITerrainErosion::SetNumIteration(int32 InNumIteration)
{
	NumIteration = InNumIteration;
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
	/*
	 * 四个方向上的流量，顺序是L、R、T、B
	 */
	FluxChannel = Data->GetChannel("flux");
	ApplyInitialization();
	for(int32 Iterate = 0; Iterate < NumIteration; Iterate++)
	{
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
			float LValue = i == 0? 0: (HeightChannel->GetValue(i - 1, j)->GetNumber() + WaterChannel->GetValue(i - 1, j)->GetNumber());
			float RValue = i == SizeX - 1? 0: (HeightChannel->GetValue(i + 1, j)->GetNumber() + WaterChannel->GetValue(i + 1, j)->GetNumber());
			float TValue = j == 0? 0: (HeightChannel->GetValue(i, j - 1)->GetNumber() + WaterChannel->GetValue(i, j - 1)->GetNumber());
			float BValue = j == SizeY - 1? 0: (HeightChannel->GetValue(i, j + 1)->GetNumber() + WaterChannel->GetValue(i, j + 1)->GetNumber());
			float Value = HeightChannel->GetValue(i, j)->GetNumber() + WaterChannel->GetValue(i, j)->GetNumber();
			FQuat FluxValue;
			FluxValue.X = FMath::Max(0.0f, Value - LValue);
			FluxValue.Y = FMath::Max(0.0f, Value - RValue);
			FluxValue.Z = FMath::Max(0.0f, Value - TValue);
			FluxValue.W = FMath::Max(0.0f, Value - BValue);
			FluxValue /= (FluxValue.X + FluxValue.Y + FluxValue.Z + FluxValue.W);
			FluxChannel->GetValue(i, j)->SetFQuat(FluxValue);
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
			WaterChannel->SetValue(i, j, WaterChannel->GetValue(i, j)->GetNumber() + RainAmount);
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
			float LValue = i == 0? 0: (HeightChannel->GetValue(i - 1, j)->GetNumber() + WaterChannel->GetValue(i - 1, j)->GetNumber());
			float RValue = i == SizeX - 1? 0: (HeightChannel->GetValue(i + 1, j)->GetNumber() + WaterChannel->GetValue(i + 1, j)->GetNumber());
			float TValue = j == 0? 0: (HeightChannel->GetValue(i, j - 1)->GetNumber() + WaterChannel->GetValue(i, j - 1)->GetNumber());
			float BValue = j == SizeY - 1? 0: (HeightChannel->GetValue(i, j + 1)->GetNumber() + WaterChannel->GetValue(i, j + 1)->GetNumber());
			float Value = HeightChannel->GetValue(i, j)->GetNumber() + WaterChannel->GetValue(i, j)->GetNumber();
			FQuat FluxValue;
			FluxValue.X = FMath::Max(0.0f, Value - LValue + FluxChannel->GetValue(i, j)->GetFQuat().X);
			FluxValue.Y = FMath::Max(0.0f, Value - RValue + FluxChannel->GetValue(i, j)->GetFQuat().Y);
			FluxValue.Z = FMath::Max(0.0f, Value - TValue + FluxChannel->GetValue(i, j)->GetFQuat().Z);
			FluxValue.W = FMath::Max(0.0f, Value - BValue + FluxChannel->GetValue(i, j)->GetFQuat().W);
			float K = FMath::Max(1.0f, WaterChannel->GetValue(i, j)->GetNumber() / (FluxValue.X + FluxValue.Y + FluxValue.Z + FluxValue.W));
			FluxValue /= K;
			FluxChannel->SetValue(i, j, FluxValue);
		}
	}
	/*
	 * 根据流量算之后的水量
	 */
	for(int32 i = 0; i < SizeX; i++)
	{
		for(int32 j = 0; j < SizeY; j++)
		{
			FQuat FluxValue = FluxChannel->GetValue(i, j)->GetFQuat();
			float OutValue = FluxValue.X + FluxValue.Y + FluxValue.Z + FluxValue.W;
			float LInValue = i == 0? 0: FluxChannel->GetValue(i - 1, j)->GetFQuat().Y;
			float RInValue = i == SizeX - 1? 0: FluxChannel->GetValue(i + 1, j)->GetFQuat().X;
			float TInValue = j == 0? 0: FluxChannel->GetValue(i, j - 1)->GetFQuat().W;
			float BInValue = j == SizeY - 1? 0: FluxChannel->GetValue(i, j + 1)->GetFQuat().Z;
			float DeltaWaterValue = LInValue + RInValue + TInValue + BInValue - OutValue;
			WaterChannel->SetValue(i, j, WaterChannel->GetValue(i, j)->GetNumber() + DeltaWaterValue);
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
			FQuat FluxValue = FluxChannel->GetValue(i, j)->GetFQuat();
			float LInValue = i == 0? 0: FluxChannel->GetValue(i - 1, j)->GetFQuat().Y;
			float RInValue = i == SizeX - 1? 0: FluxChannel->GetValue(i + 1, j)->GetFQuat().X;
			float TInValue = j == 0? 0: FluxChannel->GetValue(i, j - 1)->GetFQuat().W;
			float BInValue = j == SizeY - 1? 0: FluxChannel->GetValue(i, j + 1)->GetFQuat().Z;
			FVector Velocity;
			Velocity.X = (LInValue - FluxValue.X + FluxValue.Y - RInValue) / 2;
			Velocity.Y = (TInValue - FluxValue.Z + FluxValue.W - BInValue) / 2;
			VelocityChannel->SetValue(i, j, Velocity);
			float VelocityValue = FMath::Sqrt(Velocity.X * Velocity.X + Velocity.Y * Velocity.Y);
			float SedimentCapacity = FMath::Sin(HydroErosionAngle) * VelocityValue;
			float SedimentValue = SedimentChannel->GetValue(i, j)->GetNumber();
			float NewHeight = HeightChannel->GetValue(i, j)->GetNumber();
			float Sediment1 = SedimentChannel->GetValue(i, j)->GetNumber();
			if(SedimentValue > SedimentCapacity)
			{
				NewHeight += (SedimentValue - SedimentCapacity);
				Sediment1 -= (SedimentValue - SedimentCapacity);
			}
			else
			{
				NewHeight -= (SedimentCapacity - SedimentValue);
				Sediment1 += (SedimentCapacity - SedimentValue);
			}
			HeightChannel->SetValue(i, j, NewHeight);
			SedimentChannel->SetValue(i, j, Sediment1);
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
			FVector Velocity = VelocityChannel->GetValue(i, j)->GetFVector();
			float FromPosX = i - Velocity.X;	// 缺个dt
			float FromPosY = j - Velocity.Y;	// 同上
			int32 X0 = FMath::FloorToInt(FromPosX);
			int32 Y0 = FMath::FloorToInt(FromPosY);
			int32 X1 = X0 + 1;
			int32 Y1 = Y0 + 1;
			X0 = FMath::Clamp(X0, 0, SizeX - 1);
			Y0 = FMath::Clamp(Y0, 0, SizeY - 1);
			X1 = FMath::Clamp(X1, 0, SizeX - 1);
			Y1 = FMath::Clamp(Y1, 0, SizeY - 1);
			float S00 = SedimentChannel->GetValue(X0, Y0)->GetNumber();
			float S01 = SedimentChannel->GetValue(X0, Y1)->GetNumber();
			float S10 = SedimentChannel->GetValue(X1, Y0)->GetNumber();
			float S11 = SedimentChannel->GetValue(X1, Y1)->GetNumber();
			int32 AlphaX = FromPosX - FMath::FloorToInt(FromPosX);
			int32 AlphaY = FromPosY - FMath::FloorToInt(FromPosY);
			float NewSediment = UHITerrainMathMisc::LinearLerp2D(S00, S01, S10, S11, AlphaX, AlphaY);
			SedimentChannel->SetValue(i, j, NewSediment);
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
			float WaterValue = WaterChannel->GetValue(i, j)->GetNumber();
			WaterValue *= (1 - EvaporationAmount);
			WaterChannel->SetValue(i, j, WaterValue);
		}
	}
}
