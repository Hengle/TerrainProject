// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainMaths/Modules/HITerrainErosionGPU.h"

#include "RenderGraphUtils.h"
#include "RenderTargetPool.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Rendering/Texture2DResource.h"
#include "Async/Async.h"

IMPLEMENT_GLOBAL_SHADER(FErosionShader, "/TerrainShaders/TestComputeShader.usf", "MainComputeShader", SF_Compute);


FHITerrainErosionGPU::FHITerrainErosionGPU():NumIteration(120), DeltaTime(1.0f / 60),bEnableHydroErosion(true), bEnableThermalErosion(true),
	HydroErosionScale(1.0), RainAmount(1000.0),
	EvaporationAmount(0.2), HydroErosionAngle(50), ErosionScale(0.012), DepositionScale(0.012), SedimentCapacityScale(1),
	ThermalErosionScale(1.0)
	
{
	
}

FHITerrainErosionGPU::~FHITerrainErosionGPU()
{
	
}

void FHITerrainErosionGPU::SetNumIteration(int32 InNumIteration)
{
	NumIteration = InNumIteration;
}

void FHITerrainErosionGPU::SetDeltaTime(float InDeltaTime)
{
	DeltaTime = InDeltaTime;
}

void FHITerrainErosionGPU::SetEnableHydroErosion(bool InBool)
{
	bEnableHydroErosion = InBool;
}

void FHITerrainErosionGPU::SetEnableThermalErosion(bool InBool)
{
	bEnableThermalErosion = InBool;
}

void FHITerrainErosionGPU::SetHydroErosionScale(float InHydroErosionScale)
{
	HydroErosionScale = InHydroErosionScale;
}

void FHITerrainErosionGPU::SetRainAmount(float InRainAmount)
{
	RainAmount = InRainAmount;
}

void FHITerrainErosionGPU::SetEvaporationAmount(float InEvaporationAmount)
{
	EvaporationAmount = InEvaporationAmount;
}

void FHITerrainErosionGPU::SetHydroErosionAngle(float InHydroErosionAngle)
{
	HydroErosionAngle = InHydroErosionAngle;
}

void FHITerrainErosionGPU::SetErosionScale(float InErosionScale)
{
	ErosionScale = InErosionScale;
}

void FHITerrainErosionGPU::SetDepositionScale(float InDepositionScale)
{
	DepositionScale = InDepositionScale;
}

void FHITerrainErosionGPU::SetSedimentCapacityScale(float InSedimentCapacityScale)
{
	SedimentCapacityScale = InSedimentCapacityScale;
}

void FHITerrainErosionGPU::SetThermalErosionScale(float InThermalErosionScale)
{
	ThermalErosionScale = InThermalErosionScale;
}

void FHITerrainErosionGPU::ApplyModule(UHITerrainData* Data)
{
	Data->AddChannel("water", ETerrainDataType::FLOAT);
	Data->AddChannel("sediment", ETerrainDataType::FLOAT);
	Data->AddChannel("hardness", ETerrainDataType::FLOAT);
	ApplyErosionShader(Data);
}

void FHITerrainErosionGPU::ApplyErosionShader(UHITerrainData* Data)
{
	FRHICommandListImmediate& RHICmdList = GRHICommandList.GetImmediateCommandList();
	
	int32 Size = Data->Size();
			
	TResourceArray<float> HeightBuffer;
	TResourceArray<float> WaterBuffer;
	TResourceArray<float> SedimentBuffer;
	TResourceArray<float> HardnessBuffer;

	TResourceArray<float> FluxBuffer;
	TResourceArray<float> TerrainFluxBuffer;
	TResourceArray<float> VelocityBuffer;
	
	for(int32 i = 0; i < Size; i++)
	{
		for(int32 j = 0; j < Size; j++)
		{
			HeightBuffer.Add(Data->GetChannel("height")->GetFloat(i, j));
			WaterBuffer.Add(Data->GetChannel("water")->GetFloat(i, j));
			SedimentBuffer.Add(Data->GetChannel("sediment")->GetFloat(i, j));
			HardnessBuffer.Add(Data->GetChannel("hardness")->GetFloat(i, j));

			FluxBuffer.Add(0.0f);
			FluxBuffer.Add(0.0f);
			FluxBuffer.Add(0.0f);
			FluxBuffer.Add(0.0f);

			TerrainFluxBuffer.Add(0.0f);
			TerrainFluxBuffer.Add(0.0f);
			TerrainFluxBuffer.Add(0.0f);
			TerrainFluxBuffer.Add(0.0f);

			VelocityBuffer.Add(0.0f);
			VelocityBuffer.Add(0.0f);
			VelocityBuffer.Add(0.0f);
		}
	}
	HeightBuffer.SetAllowCPUAccess(true);
	WaterBuffer.SetAllowCPUAccess(true);
	SedimentBuffer.SetAllowCPUAccess(true);
	HardnessBuffer.SetAllowCPUAccess(true);
	
	FRHIResourceCreateInfo HeightCreateInfo;
	HeightCreateInfo.ResourceArray = &HeightBuffer;
	FRHIResourceCreateInfo WaterCreateInfo;
	WaterCreateInfo.ResourceArray = &WaterBuffer;
	FRHIResourceCreateInfo SedimentCreateInfo;
	SedimentCreateInfo.ResourceArray = &SedimentBuffer;
	FRHIResourceCreateInfo HardnessCreateInfo;
	HardnessCreateInfo.ResourceArray = &HardnessBuffer;
	FRHIResourceCreateInfo FluxCreateInfo;
	FluxCreateInfo.ResourceArray = &FluxBuffer;
	FRHIResourceCreateInfo TerrainFluxCreateInfo;
	TerrainFluxCreateInfo.ResourceArray = &TerrainFluxBuffer;
	FRHIResourceCreateInfo VelocityCreateInfo;
	VelocityCreateInfo.ResourceArray = &VelocityBuffer;
	
	FStructuredBufferRHIRef HeightRHIRef = RHICreateStructuredBuffer(sizeof(float), sizeof(float) * Size * Size, BUF_UnorderedAccess | BUF_ShaderResource, HeightCreateInfo);
	FUnorderedAccessViewRHIRef HeightUAVRef = RHICreateUnorderedAccessView(HeightRHIRef, true, false);
	FStructuredBufferRHIRef WaterRHIRef = RHICreateStructuredBuffer(sizeof(float), sizeof(float) * Size * Size, BUF_UnorderedAccess | BUF_ShaderResource, WaterCreateInfo);
	FUnorderedAccessViewRHIRef WaterUAVRef = RHICreateUnorderedAccessView(WaterRHIRef, true, false);
	FStructuredBufferRHIRef SedimentRHIRef = RHICreateStructuredBuffer(sizeof(float), sizeof(float) * Size * Size, BUF_UnorderedAccess | BUF_ShaderResource, SedimentCreateInfo);
	FUnorderedAccessViewRHIRef SedimentUAVRef = RHICreateUnorderedAccessView(SedimentRHIRef, true, false);
	FStructuredBufferRHIRef HardnessRHIRef = RHICreateStructuredBuffer(sizeof(float), sizeof(float) * Size * Size, BUF_UnorderedAccess | BUF_ShaderResource, HardnessCreateInfo);
	FUnorderedAccessViewRHIRef HardnessUAVRef = RHICreateUnorderedAccessView(HardnessRHIRef, true, false);
	FStructuredBufferRHIRef FluxRHIRef = RHICreateStructuredBuffer(sizeof(float), sizeof(float) * Size * Size * 4, BUF_UnorderedAccess | BUF_ShaderResource, FluxCreateInfo);
	FUnorderedAccessViewRHIRef FluxUAVRef = RHICreateUnorderedAccessView(FluxRHIRef, true, false);
	FStructuredBufferRHIRef TerrainFluxRHIRef = RHICreateStructuredBuffer(sizeof(float), sizeof(float) * Size * Size * 4, BUF_UnorderedAccess | BUF_ShaderResource, TerrainFluxCreateInfo);
	FUnorderedAccessViewRHIRef TerrainFluxUAVRef = RHICreateUnorderedAccessView(TerrainFluxRHIRef, true, false);
	FStructuredBufferRHIRef VelocityRHIRef = RHICreateStructuredBuffer(sizeof(float), sizeof(float) * Size * Size * 3, BUF_UnorderedAccess | BUF_ShaderResource, VelocityCreateInfo);
	FUnorderedAccessViewRHIRef VelocityUAVRef = RHICreateUnorderedAccessView(VelocityRHIRef, true, false);

	FErosionShader::FParameters Parameters;
	Parameters.Size = Size;
	Parameters.NumIteration = NumIteration;
	Parameters.DeltaTime = DeltaTime;
	Parameters.GBEnableHydroErosion = bEnableHydroErosion;
	Parameters.GBEnableThermalErosion = bEnableThermalErosion;
	Parameters.HydroErosionScale = HydroErosionScale;
	Parameters.RainAmount = RainAmount;
	Parameters.EvaporationAmount = EvaporationAmount;
	Parameters.HydroErosionAngle = HydroErosionAngle;
	Parameters.ErosionScale = ErosionScale;
	Parameters.DepositionScale = DepositionScale;
	Parameters.SedimentCapacityScale = SedimentCapacityScale;
	Parameters.ThermalErosionScale = ThermalErosionScale;
	Parameters.Height = HeightUAVRef;
	Parameters.Water = WaterUAVRef;
	Parameters.Sediment = SedimentUAVRef;
	Parameters.Hardness = HardnessUAVRef;
	Parameters.Flux = FluxUAVRef;
	Parameters.TerrainFlux = TerrainFluxUAVRef;
	Parameters.Velocity = VelocityUAVRef;


	
	// AsyncTask(ENamedThreads::GameThread, []()
	// {
	// 	FlushRenderingCommands();
	// });

	ENQUEUE_RENDER_COMMAND(ErosionModuleCommand)(
		[=](FRHICommandListImmediate& RHICmdList){
			TShaderMapRef<FErosionShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, Parameters, 
								FIntVector(Size / 8,
										Size / 8, 1));

			AsyncTask(ENamedThreads::GameThread, []()
			{
				FlushRenderingCommands();
			});
			
			float* HeightSrc = (float*)RHICmdList.LockStructuredBuffer(HeightRHIRef.GetReference(), 0, sizeof(float) * Size * Size, EResourceLockMode::RLM_ReadOnly);
			float* WaterSrc = (float*)RHICmdList.LockStructuredBuffer(WaterRHIRef.GetReference(), 0, sizeof(float) * Size * Size, EResourceLockMode::RLM_ReadOnly);
			float* SedimentSrc = (float*)RHICmdList.LockStructuredBuffer(SedimentRHIRef.GetReference(), 0, sizeof(float) * Size * Size, EResourceLockMode::RLM_ReadOnly);
			float* HardnessSrc = (float*)RHICmdList.LockStructuredBuffer(HardnessRHIRef.GetReference(), 0, sizeof(float) * Size * Size, EResourceLockMode::RLM_ReadOnly);

			TArray<float> ResultHeight;
			ResultHeight.Reserve(Size * Size);
			ResultHeight.AddUninitialized(Size * Size);
			TArray<float> ResultWater;
			ResultWater.Reserve(Size * Size);
			ResultWater.AddUninitialized(Size * Size);
			TArray<float> ResultSediment;
			ResultSediment.Reserve(Size * Size);
			ResultSediment.AddUninitialized(Size * Size);
			TArray<float> ResultHardness;
			ResultHardness.Reserve(Size * Size);
			ResultHardness.AddUninitialized(Size * Size);

			FMemory::Memcpy(ResultHeight.GetData(), HeightSrc, sizeof(float) * Size * Size);
			FMemory::Memcpy(ResultWater.GetData(), WaterSrc, sizeof(float) * Size * Size);
			FMemory::Memcpy(ResultSediment.GetData(), SedimentSrc, sizeof(float) * Size * Size);
			FMemory::Memcpy(ResultHardness.GetData(), HardnessSrc, sizeof(float) * Size * Size);
			
			// for(int32 i = 0; i < Size; i++)
			// {
			// 	for(int32 j = 0; j < Size; j++)
			// 	{
			// 		ResultHeight.Add(*HeightSrc);
			// 		ResultWater.Add(*WaterSrc);
			// 		ResultSediment.Add(*SedimentSrc);
			// 		ResultHardness.Add(*HardnessSrc);
			// 		HeightSrc++;
			// 		WaterSrc++;
			// 		SedimentSrc++;
			// 		HardnessSrc++;
			// 	}
			// }
			RHICmdList.UnlockStructuredBuffer(HeightRHIRef.GetReference());
			RHICmdList.UnlockStructuredBuffer(WaterRHIRef.GetReference());
			RHICmdList.UnlockStructuredBuffer(SedimentRHIRef.GetReference());
			RHICmdList.UnlockStructuredBuffer(HardnessRHIRef.GetReference());

			for(int32 i = 0; i < Size; i++)
			{
				for(int32 j = 0; j < Size; j++)
				{
					int32 Index = i * Size + j;
					Data->SetChannelValue("height", i, j, ResultHeight[Index]);
					Data->SetChannelValue("water", i, j, ResultWater[Index]);
					Data->SetChannelValue("sediment", i, j, ResultSediment[Index]);
					Data->SetChannelValue("hardness", i, j, ResultHardness[Index]);
				}
			}
		}
	);
	
}
