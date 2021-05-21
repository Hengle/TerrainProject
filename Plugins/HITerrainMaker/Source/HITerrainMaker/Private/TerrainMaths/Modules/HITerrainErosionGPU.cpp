// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainMaths/Modules/HITerrainErosionGPU.h"

#include "RenderGraphUtils.h"
#include "RenderTargetPool.h"
#include "Async/Async.h"

IMPLEMENT_GLOBAL_SHADER(FErosionShaderRain, "/TerrainShaders/ErosionShader1_Rain.usf", "Main", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FErosionShaderCalcFlow, "/TerrainShaders/ErosionShader2_CalcFlow.usf", "Main", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FErosionShaderApplyFlow, "/TerrainShaders/ErosionShader3_ApplyFlow.usf", "Main", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FErosionShaderErosionDeposition, "/TerrainShaders/ErosionShader4_ErosionDeposition.usf", "Main", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FErosionShaderErosionDeposition2, "/TerrainShaders/ErosionShader4_ErosionDeposition2.usf", "Main", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FErosionShaderSedimentFlow, "/TerrainShaders/ErosionShader5_SedimentFlow.usf", "Main", SF_Compute);

FHITerrainErosionGPU::FHITerrainErosionGPU():NumIteration(1200), DeltaTime(0.02),bEnableHydroErosion(true), bEnableThermalErosion(true),
	HydroErosionScale(0), RainAmount(10.0f),
	EvaporationAmount(0.05), ErosionScale(0.008), DepositionScale(0.016), SedimentCapacityScale(1),
	ThermalErosionScale(0.3)
	
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
	for(int32 i = 0; i < Data->RealSize(); i++)
	{
		for(int32 j = 0; j < Data->RealSize(); j++)
		{
			Data->SetChannelValue("hardness", i, j, 1.0f);
		}
	}
	ApplyErosionShader(Data);
}

void FHITerrainErosionGPU::ApplyErosionShader(UHITerrainData* Data)
{
	FRHICommandListImmediate& RHICmdList = GRHICommandList.GetImmediateCommandList();
	// Data->Mutex.Lock();
	
	ENQUEUE_RENDER_COMMAND(ErosionModuleCommand)(
		[=](FRHICommandListImmediate& RHICmdList)
		{
			
			int32 Size = Data->RealSize();
			
			TResourceArray<float> TerrainDataBuffer;
			TResourceArray<float> FluxBuffer;
			TResourceArray<float> TerrainFluxBuffer;
			TResourceArray<float> VelocityBuffer;

			TResourceArray<float> TempTerrainDataBuffer;

			
			for(int32 i = 0; i < Size; i++)
			{
				for(int32 j = 0; j < Size; j++)
				{
					TerrainDataBuffer.Add(Data->GetChannel("height")->GetFloat(i, j));
					TerrainDataBuffer.Add(Data->GetChannel("water")->GetFloat(i, j));
					TerrainDataBuffer.Add(Data->GetChannel("sediment")->GetFloat(i, j));
					TerrainDataBuffer.Add(Data->GetChannel("hardness")->GetFloat(i, j));

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

					TempTerrainDataBuffer.Add(Data->GetChannel("height")->GetFloat(i, j));
					TempTerrainDataBuffer.Add(Data->GetChannel("water")->GetFloat(i, j));
					TempTerrainDataBuffer.Add(Data->GetChannel("sediment")->GetFloat(i, j));
					TempTerrainDataBuffer.Add(Data->GetChannel("hardness")->GetFloat(i, j));
				}
			}
			TerrainDataBuffer.SetAllowCPUAccess(true);
			
			FRHIResourceCreateInfo TerrainDataCreateInfo;
			TerrainDataCreateInfo.ResourceArray = &TerrainDataBuffer;
			FRHIResourceCreateInfo FluxCreateInfo;
			FluxCreateInfo.ResourceArray = &FluxBuffer;
			FRHIResourceCreateInfo TerrainFluxCreateInfo;
			TerrainFluxCreateInfo.ResourceArray = &TerrainFluxBuffer;
			FRHIResourceCreateInfo VelocityCreateInfo;
			VelocityCreateInfo.ResourceArray = &VelocityBuffer;

			FRHIResourceCreateInfo TempTerrainDataCreateInfo;
			TempTerrainDataCreateInfo.ResourceArray = &TempTerrainDataBuffer;
			
			FStructuredBufferRHIRef TerrainDataRHIRef = RHICreateStructuredBuffer(sizeof(float), sizeof(float) * Size * Size * 4, BUF_UnorderedAccess | BUF_ShaderResource, TerrainDataCreateInfo);
			FUnorderedAccessViewRHIRef TerrainDataUAVRef = RHICreateUnorderedAccessView(TerrainDataRHIRef, true, false);
			FStructuredBufferRHIRef FluxRHIRef = RHICreateStructuredBuffer(sizeof(float), sizeof(float) * Size * Size * 4, BUF_UnorderedAccess | BUF_ShaderResource, FluxCreateInfo);
			FUnorderedAccessViewRHIRef FluxUAVRef = RHICreateUnorderedAccessView(FluxRHIRef, true, false);
			FStructuredBufferRHIRef TerrainFluxRHIRef = RHICreateStructuredBuffer(sizeof(float), sizeof(float) * Size * Size * 4, BUF_UnorderedAccess | BUF_ShaderResource, TerrainFluxCreateInfo);
			FUnorderedAccessViewRHIRef TerrainFluxUAVRef = RHICreateUnorderedAccessView(TerrainFluxRHIRef, true, false);
			FStructuredBufferRHIRef VelocityRHIRef = RHICreateStructuredBuffer(sizeof(float), sizeof(float) * Size * Size * 2, BUF_UnorderedAccess | BUF_ShaderResource, VelocityCreateInfo);
			FUnorderedAccessViewRHIRef VelocityUAVRef = RHICreateUnorderedAccessView(VelocityRHIRef, true, false);

			FStructuredBufferRHIRef TempTerrainDataRHIRef = RHICreateStructuredBuffer(sizeof(float), sizeof(float) * Size * Size * 4, BUF_UnorderedAccess | BUF_ShaderResource, TempTerrainDataCreateInfo);
			FUnorderedAccessViewRHIRef TempTerrainDataUAVRef = RHICreateUnorderedAccessView(TempTerrainDataRHIRef, true, false);
		
			TShaderMapRef<FErosionShaderRain> ComputeShader1_Rain(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			FErosionShaderRain::FParameters Parameters1;
			Parameters1.Size = Size;
			Parameters1.DeltaTime = DeltaTime;
			Parameters1.RainAmount = RainAmount;
			Parameters1.TerrainData = TerrainDataUAVRef;

			TShaderMapRef<FErosionShaderCalcFlow> ComputeShader2_CalcFlow(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			FErosionShaderCalcFlow::FParameters Parameters2;
			Parameters2.Size = Size;
			Parameters2.DeltaTime = DeltaTime;
			Parameters2.TerrainData = TerrainDataUAVRef;
			Parameters2.Flux = FluxUAVRef;
			
			TShaderMapRef<FErosionShaderApplyFlow> ComputeShader3_ApplyFlow(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			FErosionShaderApplyFlow::FParameters Parameters3;
			Parameters3.Size = Size;
			Parameters3.DeltaTime = DeltaTime;
			Parameters3.TerrainData = TerrainDataUAVRef;
			Parameters3.Flux = FluxUAVRef;
			
			TShaderMapRef<FErosionShaderErosionDeposition> ComputeShader4_ErosionDeposition(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			FErosionShaderErosionDeposition::FParameters Parameters4;
			Parameters4.Size = Size;
			Parameters4.DeltaTime = DeltaTime;
			Parameters4.ErosionScale = ErosionScale;
			Parameters4.DepositionScale = DepositionScale;
			Parameters4.TerrainData = TerrainDataUAVRef;
			Parameters4.Flux = FluxUAVRef;
			Parameters4.Velocity = VelocityUAVRef;
			Parameters4.TempTerrainData = TempTerrainDataUAVRef;

			TShaderMapRef<FErosionShaderErosionDeposition2> ComputeShader4_ErosionDeposition2(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			FErosionShaderErosionDeposition2::FParameters Parameters4_2;
			Parameters4_2.Size = Size;
			Parameters4_2.TerrainData = TerrainDataUAVRef;
			Parameters4_2.TempTerrainData = TempTerrainDataUAVRef;

			TShaderMapRef<FErosionShaderSedimentFlow> ComputeShader5_SedimentFlow(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			FErosionShaderSedimentFlow::FParameters Parameters5;
			Parameters5.Size = Size;
			Parameters5.DeltaTime = DeltaTime;
			Parameters5.TerrainData = TerrainDataUAVRef;
			Parameters5.Velocity = VelocityUAVRef;

			for(int32 i = 0; i < NumIteration; i++)
			{
				// if(i < NumIteration / 2)
				{
					FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader1_Rain, Parameters1, FIntVector(Size / 8, Size / 8, 1));
				}
				FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader2_CalcFlow, Parameters2, FIntVector(Size / 8, Size / 8, 1));
				FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader3_ApplyFlow, Parameters3, FIntVector(Size / 8, Size / 8, 1));
				FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader4_ErosionDeposition, Parameters4, FIntVector(Size / 8, Size / 8, 1));
				FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader4_ErosionDeposition2, Parameters4_2, FIntVector(Size / 8, Size / 8, 1));
				FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader5_SedimentFlow, Parameters5, FIntVector(Size / 8, Size / 8, 1));
				
			}
			for(int32 i = 0; i < NumIteration * 2; i++)
			{
				FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader2_CalcFlow, Parameters2, FIntVector(Size / 8, Size / 8, 1));
				FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader3_ApplyFlow, Parameters3, FIntVector(Size / 8, Size / 8, 1));
			}
			RHICmdList.SubmitCommandsAndFlushGPU();
			float* TerrainDataSrc = (float*)RHICmdList.LockStructuredBuffer(TerrainDataRHIRef.GetReference(), 0, sizeof(float) * Size * Size * 4, EResourceLockMode::RLM_ReadOnly);

			TArray<float> ResultTerrainData;
			ResultTerrainData.Reserve(Size * Size * 4);
			ResultTerrainData.AddUninitialized(Size * Size * 4);

			FMemory::Memcpy(ResultTerrainData.GetData(), TerrainDataSrc, sizeof(float) * Size * Size * 4);
			
			RHICmdList.UnlockStructuredBuffer(TerrainDataRHIRef.GetReference());

			for(int32 i = 0; i < Size; i++)
			{
				for(int32 j = 0; j < Size; j++)
				{
					int32 Index = (i * Size + j) * 4;
					Data->SetChannelValue("height", i, j, ResultTerrainData[Index]);
					Data->SetChannelValue("water", i, j, ResultTerrainData[Index + 1]);
					Data->SetChannelValue("sediment", i, j, ResultTerrainData[Index + 2]);
					Data->SetChannelValue("hardness", i, j, ResultTerrainData[Index + 3]);
				}
			}
			TerrainDataRHIRef.SafeRelease();
			TerrainDataUAVRef.SafeRelease();
			FluxRHIRef.SafeRelease();
			FluxUAVRef.SafeRelease();
			TerrainFluxRHIRef.SafeRelease();
			TerrainFluxUAVRef.SafeRelease();
			VelocityRHIRef.SafeRelease();
			VelocityUAVRef.SafeRelease();
			TempTerrainDataRHIRef.SafeRelease();
			TempTerrainDataUAVRef.SafeRelease();
			
			// Data->Mutex.Unlock();
		});
}
