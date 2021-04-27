// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainMaths/Modules/HITerrainErosion.h"

#include "TerrainMaths/HITerrainMathMisc.h"

const float Gravity = 9.8f;

FHITerrainErosion::FHITerrainErosion():NumIteration(20), DeltaTime(1.0f / 60), HydroErosionScale(1.0), RainAmount(1000.0),
	EvaporationAmount(0.2), HydroErosionAngle(50), ErosionScale(0.012), DepositionScale(0.012), SedimentCapacityScale(1),
	NumFlowIteration(10),
	ThermalErosionScale(1.0)
{
	
}

FHITerrainErosion::~FHITerrainErosion()
{
	
}

void FHITerrainErosion::SetSeed(int32 InSeed)
{
	Seed = InSeed;
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

void FHITerrainErosion::SetNumFlowIteration(int32 InNumFlowIteration)
{
	NumFlowIteration = InNumFlowIteration;
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
	Data->AddChannel("terrainflux", ETerrainDataType::FQUAT);
	HeightChannel = Data->GetChannel("height");
	WaterChannel = Data->GetChannel("water");
	SedimentChannel = Data->GetChannel("sediment");
	VelocityChannel = Data->GetChannel("velocity");
	FluxChannel = Data->GetChannel("flux");
	TerrainFluxChannel = Data->GetChannel("terrainflux");
	/*
	 * 四个方向上的流量，顺序是L、R、T、B
	 */
	ApplyInitialization();
	// ApplyRainSimulation();
	for(int32 Iterate = 0; Iterate < NumIteration; Iterate++)
	{
		UE_LOG(LogHITerrain, Warning, TEXT("Erosion Iteration %d"), Iterate);
		if(Iterate < NumIteration / 2)
		{
			ApplyRainSimulation();
		}
		
		for(int32 FlowIterate = 0; FlowIterate < NumFlowIteration; FlowIterate++)
		{
			ApplyFlowSimulation();
			ApplyErosionDepositionSimulation();
		}
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
			FQuat FluxValue(0.0f, 0.0f, 0.0f, 0.0f);
			FluxChannel->SetFQuat(i, j, FluxValue);
		}
	}
	/*
	 * 初始化Velocity通道
	 */
	for(int32 i = 0; i < SizeX; i++)
	{
		for(int32 j = 0; j < SizeY; j++)
		{
			FVector VelocityValue(0.0f, 0.0f, 0.0f);
			VelocityChannel->SetFVector(i, j, VelocityValue);
		}
	}
}

/*
 * 模拟一个降雨
 */
void FHITerrainErosion::ApplyRainSimulation()
{
	int32 SizeX = HeightChannel->GetSizeX();
	int32 SizeY = HeightChannel->GetSizeY();
	FRandomStream RandomStream(Seed);
	for(int32 i = 0; i < SizeX; i++)
	{
		for(int32 j = 0; j < SizeY; j++)
		{
			float RainBool = RandomStream.FRand() < 0.5? 0: 1;
			// WaterChannel->SetFloat(i, j, WaterChannel->GetFloat(i, j) + RainAmount * DeltaTime * RainBool);
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
			FluxValue.X = FMath::Max(0.0f, (Value - LValue) * DeltaTime * Gravity + FluxChannel->GetFQuat(i, j).X);
			FluxValue.Y = FMath::Max(0.0f, (Value - RValue) * DeltaTime * Gravity + FluxChannel->GetFQuat(i, j).Y);
			FluxValue.Z = FMath::Max(0.0f, (Value - TValue) * DeltaTime * Gravity + FluxChannel->GetFQuat(i, j).Z);
			FluxValue.W = FMath::Max(0.0f, (Value - BValue) * DeltaTime * Gravity + FluxChannel->GetFQuat(i, j).W);
			float K = FMath::Min(1.0f, WaterChannel->GetFloat(i, j) / ((FluxValue.X + FluxValue.Y + FluxValue.Z + FluxValue.W) * DeltaTime));
			K = FMath::Max(0.0f, K);
			FluxValue *= K;
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
			float LHeight = i == 0? HeightChannel->GetFloat(i, j): HeightChannel->GetFloat(i - 1, j);
			float RHeight = i == SizeX - 1? HeightChannel->GetFloat(i, j): HeightChannel->GetFloat(i + 1, j);
			float THeight = j == 0? HeightChannel->GetFloat(i, j): HeightChannel->GetFloat(i, j - 1);
			float BHeight = j == SizeY - 1? HeightChannel->GetFloat(i, j): HeightChannel->GetFloat(i, j + 1);
			float TiltAngle = FMath::Sqrt((LHeight - RHeight) * (LHeight - RHeight) / 4 + (THeight + BHeight) * (THeight + BHeight) / 4);
			TiltAngle /= FMath::Sqrt(1 + (LHeight - RHeight) * (LHeight - RHeight) / 4 + (THeight + BHeight) * (THeight + BHeight) / 4);
			// float SedimentCapacity = FMath::Sin(HydroErosionAngle) * VelocityValue;
			float SedimentCapacity = TiltAngle * VelocityValue;
			float SedimentValue = SedimentChannel->GetFloat(i, j);
			float Sediment = SedimentChannel->GetFloat(i, j);
			float Height = HeightChannel->GetFloat(i, j);
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
			HeightChannel->SetFloat(i, j, Height);
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
			float NewSediment = FHITerrainMathMisc::LinearLerp2D(S00, S01, S10, S11, AlphaX, AlphaY);
			SedimentChannel->SetFloat(i, j, NewSediment);
		}
	}
}

void FHITerrainErosion::ApplyThermalErosionSimulation()
{
	int32 SizeX = HeightChannel->GetSizeX();
	int32 SizeY = HeightChannel->GetSizeY();
	for(int32 i = 0; i < SizeX; i++)
	{
		for(int32 j = 0; j < SizeY; j++)
		{
			float HeightValue = HeightChannel->GetFloat(i, j);
			float LHeight = i == 0? HeightChannel->GetFloat(i, j): HeightChannel->GetFloat(i - 1, j);
			float RHeight = i == SizeX - 1? HeightChannel->GetFloat(i, j): HeightChannel->GetFloat(i + 1, j);
			float THeight = j == 0? HeightChannel->GetFloat(i, j): HeightChannel->GetFloat(i, j - 1);
			float BHeight = j == SizeY - 1? HeightChannel->GetFloat(i, j): HeightChannel->GetFloat(i, j + 1);
			float LHeightDiff = FMath::Max(0.0f, HeightValue - LHeight);
			float RHeightDiff = FMath::Max(0.0f, HeightValue - RHeight);
			float THeightDiff = FMath::Max(0.0f, HeightValue - THeight);
			float BHeightDiff = FMath::Max(0.0f, HeightValue - BHeight);
			float MaxHeightDiff = FMath::Max(FMath::Max(LHeightDiff, RHeightDiff), FMath::Max(THeightDiff, BHeightDiff));
			float RemovedHeight = MaxHeightDiff * ThermalErosionScale / 2;
			float SumHeightDiff = LHeightDiff + RHeightDiff + THeightDiff + BHeightDiff;
			if(SumHeightDiff != 0.0f)
			{
				FQuat TerrainFluxValue;
				TerrainFluxValue.X = RemovedHeight * LHeightDiff / SumHeightDiff;
				TerrainFluxValue.X = RemovedHeight * RHeightDiff / SumHeightDiff;
				TerrainFluxValue.X = RemovedHeight * THeightDiff / SumHeightDiff;
				TerrainFluxValue.X = RemovedHeight * BHeightDiff / SumHeightDiff;
				TerrainFluxChannel->SetFQuat(i, j, TerrainFluxValue);
			}
			
		}
	}
	for(int32 i = 0; i < SizeX; i++)
	{
		for(int32 j = 0; j < SizeY; j++)
		{
			FQuat TerrainFluxValue = TerrainFluxChannel->GetFQuat(i, j);
			float OutValue = TerrainFluxValue.X + TerrainFluxValue.Y + TerrainFluxValue.Z + TerrainFluxValue.W;
			float LInValue = i == 0? 0: TerrainFluxChannel->GetFQuat(i - 1, j).Y;
			float RInValue = i == SizeX - 1? 0: TerrainFluxChannel->GetFQuat(i + 1, j).X;
			float TInValue = j == 0? 0: TerrainFluxChannel->GetFQuat(i, j - 1).W;
			float BInValue = j == SizeY - 1? 0: TerrainFluxChannel->GetFQuat(i, j + 1).Z;
			float DeltaTerrainValue = (LInValue + RInValue + TInValue + BInValue - OutValue) * FMath::Min(1.0f, DeltaTime * ThermalErosionScale);
			HeightChannel->SetFloat(i, j, HeightChannel->GetFloat(i, j) + DeltaTerrainValue);
		}
	}
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
