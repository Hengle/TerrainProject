// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainMaths/Modules/HITerrainErosionGPU.h"

#include "RenderGraphUtils.h"
#include "RenderTargetPool.h"
#include "Async/Async.h"


IMPLEMENT_GLOBAL_SHADER(FErosionShaderRain, "/TerrainShaders/ErosionShader1_Rain.usf", "Main", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FErosionShaderCalcFlow, "/TerrainShaders/ErosionShader2_CalcFlow.usf", "Main", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FErosionShaderApplyFlow, "/TerrainShaders/ErosionShader3_ApplyFlow.usf", "Main", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FErosionShaderCalcErosionDeposition, "/TerrainShaders/ErosionShader4_CalcErosionDeposition.usf", "Main", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FErosionShaderApplyErosionDeposition, "/TerrainShaders/ErosionShader5_ApplyErosionDeposition.usf", "Main", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FErosionShaderSedimentFlow, "/TerrainShaders/ErosionShader6_SedimentFlow.usf", "Main", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FErosionShaderEvaporation, "/TerrainShaders/ErosionShader7_Evaporation.usf", "Main", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FErosionShaderCalcThermal, "/TerrainShaders/ErosionShader8_CalcThermal.usf", "Main", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FErosionShaderApplyThermal, "/TerrainShaders/ErosionShader9_ApplyThermal.usf", "Main", SF_Compute);


FHITerrainErosionGPU::FHITerrainErosionGPU():NumIteration(1200), DeltaTime(0.02),bEnableHydroErosion(true), bEnableThermalErosion(true),
	RainAmount(10.0f),
	EvaporationAmount(0.05), ErosionScale(0.008), DepositionScale(0.016),
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

void FHITerrainErosionGPU::SetThermalErosionScale(float InThermalErosionScale)
{
	ThermalErosionScale = InThermalErosionScale;
}

void FHITerrainErosionGPU::ApplyModule(UHITerrainData* Data)
{
	while(!Data->bAvailable)
	{
		FPlatformProcess::Sleep(0.1);
	}
	Data->bAvailable = false;
	
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
			TerrainDataCreateInfo.DebugName = TEXT("TerrainData");
			TerrainData.Initialize(sizeof(float), Size * Size * 4, TerrainDataCreateInfo, 0, true, false);
			
			FRHIResourceCreateInfo FluxCreateInfo;
			FluxCreateInfo.ResourceArray = &FluxBuffer;
			FluxCreateInfo.DebugName = TEXT("Flux");
			Flux.Initialize(sizeof(float), Size * Size * 4, FluxCreateInfo, 0, true, false);
			
			FRHIResourceCreateInfo TerrainFluxCreateInfo;
			TerrainFluxCreateInfo.ResourceArray = &TerrainFluxBuffer;
			TerrainFluxCreateInfo.DebugName = TEXT("TerrainFlux");
			TerrainFlux.Initialize(sizeof(float), Size * Size * 4, TerrainFluxCreateInfo, 0, true, false);
			
			FRHIResourceCreateInfo VelocityCreateInfo;
			VelocityCreateInfo.ResourceArray = &VelocityBuffer;
			VelocityCreateInfo.DebugName = TEXT("Velocity");
			Velocity.Initialize(sizeof(float), Size * Size * 2, VelocityCreateInfo, 0, true, false);

			FRHIResourceCreateInfo TempTerrainDataCreateInfo;
			TempTerrainDataCreateInfo.ResourceArray = &TempTerrainDataBuffer;
			TempTerrainDataCreateInfo.DebugName = TEXT("TempTerrainData");
			TempTerrainData.Initialize(sizeof(float), Size * Size * 4, TempTerrainDataCreateInfo, 0, true, false);

			
			// FStructuredBufferRHIRef TerrainDataRHIRef = RHICreateStructuredBuffer(sizeof(float), sizeof(float) * Size * Size * 4, BUF_UnorderedAccess | BUF_ShaderResource, TerrainDataCreateInfo);
			// FUnorderedAccessViewRHIRef TerrainDataUAVRef = RHICreateUnorderedAccessView(TerrainDataRHIRef, true, false);
			// FStructuredBufferRHIRef FluxRHIRef = RHICreateStructuredBuffer(sizeof(float), sizeof(float) * Size * Size * 4, BUF_UnorderedAccess | BUF_ShaderResource, FluxCreateInfo);
			// FUnorderedAccessViewRHIRef FluxUAVRef = RHICreateUnorderedAccessView(FluxRHIRef, true, false);
			// FStructuredBufferRHIRef TerrainFluxRHIRef = RHICreateStructuredBuffer(sizeof(float), sizeof(float) * Size * Size * 4, BUF_UnorderedAccess | BUF_ShaderResource, TerrainFluxCreateInfo);
			// FUnorderedAccessViewRHIRef TerrainFluxUAVRef = RHICreateUnorderedAccessView(TerrainFluxRHIRef, true, false);
			// FStructuredBufferRHIRef VelocityRHIRef = RHICreateStructuredBuffer(sizeof(float), sizeof(float) * Size * Size * 2, BUF_UnorderedAccess | BUF_ShaderResource, VelocityCreateInfo);
			// FUnorderedAccessViewRHIRef VelocityUAVRef = RHICreateUnorderedAccessView(VelocityRHIRef, true, false);
			//
			// FStructuredBufferRHIRef TempTerrainDataRHIRef = RHICreateStructuredBuffer(sizeof(float), sizeof(float) * Size * Size * 4, BUF_UnorderedAccess | BUF_ShaderResource, TempTerrainDataCreateInfo);
			// FUnorderedAccessViewRHIRef TempTerrainDataUAVRef = RHICreateUnorderedAccessView(TempTerrainDataRHIRef, true, false);
			//
			TShaderMapRef<FErosionShaderRain> ComputeShader1_Rain(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			FErosionShaderRain::FParameters Parameters1;
			Parameters1.Size = Size;
			Parameters1.DeltaTime = DeltaTime;
			Parameters1.RainAmount = RainAmount;
			Parameters1.TerrainData = TerrainData.UAV;

			TShaderMapRef<FErosionShaderCalcFlow> ComputeShader2_CalcFlow(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			FErosionShaderCalcFlow::FParameters Parameters2;
			Parameters2.Size = Size;
			Parameters2.DeltaTime = DeltaTime;
			Parameters2.TerrainData = TerrainData.UAV;
			Parameters2.Flux = Flux.UAV;
			
			TShaderMapRef<FErosionShaderApplyFlow> ComputeShader3_ApplyFlow(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			FErosionShaderApplyFlow::FParameters Parameters3;
			Parameters3.Size = Size;
			Parameters3.DeltaTime = DeltaTime;
			Parameters3.TerrainData = TerrainData.UAV;
			Parameters3.Flux = Flux.UAV;
			
			TShaderMapRef<FErosionShaderCalcErosionDeposition> ComputeShader4_CalcErosionDeposition(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			FErosionShaderCalcErosionDeposition::FParameters Parameters4;
			Parameters4.Size = Size;
			Parameters4.DeltaTime = DeltaTime;
			Parameters4.ErosionScale = ErosionScale;
			Parameters4.DepositionScale = DepositionScale;
			Parameters4.TerrainData = TerrainData.UAV;
			Parameters4.Flux = Flux.UAV;
			Parameters4.Velocity = Velocity.UAV;
			Parameters4.TempTerrainData = TempTerrainData.UAV;

			TShaderMapRef<FErosionShaderApplyErosionDeposition> ComputeShader5_ApplyErosionDeposition(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			FErosionShaderApplyErosionDeposition::FParameters Parameters5;
			Parameters5.Size = Size;
			Parameters5.TerrainData = TerrainData.UAV;
			Parameters5.TempTerrainData = TempTerrainData.UAV;

			TShaderMapRef<FErosionShaderSedimentFlow> ComputeShader6_SedimentFlow(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			FErosionShaderSedimentFlow::FParameters Parameters6;
			Parameters6.Size = Size;
			Parameters6.DeltaTime = DeltaTime;
			Parameters6.TerrainData = TerrainData.UAV;
			Parameters6.Velocity = Velocity.UAV;

			TShaderMapRef<FErosionShaderEvaporation> ComputeShader7_Evaporation(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			FErosionShaderEvaporation::FParameters Parameters7;
			Parameters7.Size = Size;
			Parameters7.DeltaTime = DeltaTime;
			Parameters7.EvaporationAmount = EvaporationAmount;
			Parameters7.TerrainData = TerrainData.UAV;

			TShaderMapRef<FErosionShaderCalcThermal> ComputeShader8_CalcThermal(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			FErosionShaderCalcThermal::FParameters Parameters8;
			Parameters8.Size = Size;
			Parameters8.DeltaTime = DeltaTime;
			Parameters8.ThermalErosionScale = ThermalErosionScale;
			Parameters8.TerrainData = TerrainData.UAV;
			Parameters8.TerrainFlux = TerrainFlux.UAV;

			TShaderMapRef<FErosionShaderApplyThermal> ComputeShader9_ApplyThermal(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			FErosionShaderApplyThermal::FParameters Parameters9;
			Parameters9.Size = Size;
			Parameters9.DeltaTime = DeltaTime;
			Parameters9.TerrainData = TerrainData.UAV;
			Parameters9.TerrainFlux = TerrainFlux.UAV;

			

			for(int32 i = 0; i < NumIteration; i++)
			{
				if(bEnableHydroErosion)
				{
					FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader1_Rain, Parameters1, FIntVector(Size / 8, Size / 8, 1));
					FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader2_CalcFlow, Parameters2, FIntVector(Size / 8, Size / 8, 1));
					RHICmdList.ImmediateFlush(EImmediateFlushType::FlushRHIThreadFlushResources);
					FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader3_ApplyFlow, Parameters3, FIntVector(Size / 8, Size / 8, 1));
					// FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader4_CalcErosionDeposition, Parameters4, FIntVector(Size / 8, Size / 8, 1));
					// FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader5_ApplyErosionDeposition, Parameters5, FIntVector(Size / 8, Size / 8, 1));
					// FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader6_SedimentFlow, Parameters6, FIntVector(Size / 8, Size / 8, 1));
					// FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader7_Evaporation, Parameters7, FIntVector(Size / 8, Size / 8, 1));
				}
				if(bEnableThermalErosion)
				{
					// FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader8_CalcThermal, Parameters8, FIntVector(Size / 8, Size / 8, 1));
					// FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader9_ApplyThermal, Parameters9, FIntVector(Size / 8, Size / 8, 1));
				}
			}
			float* TerrainDataSrc = (float*)RHICmdList.LockStructuredBuffer(TerrainData.Buffer.GetReference(), 0, sizeof(float) * Size * Size * 4, EResourceLockMode::RLM_ReadOnly);
			
			TArray<float> ResultTerrainData;
			ResultTerrainData.Reserve(Size * Size * 4);
			ResultTerrainData.AddUninitialized(Size * Size * 4);

			FMemory::Memcpy(ResultTerrainData.GetData(), TerrainDataSrc, sizeof(float) * Size * Size * 4);
			
			RHICmdList.UnlockStructuredBuffer(TerrainData.Buffer.GetReference());

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

			// TerrainData.Release();
			// Flux.Release();
			// TerrainFlux.Release();
			// Velocity.Release();
			// TempTerrainData.Release();
			
			// TerrainDataRHIRef->Release();
			// TerrainDataUAVRef->Release();
			// FluxRHIRef->Release();
			// FluxUAVRef->Release();
			// TerrainFluxRHIRef->Release();
			// TerrainFluxUAVRef->Release();
			// VelocityRHIRef->Release();
			// VelocityUAVRef->Release();
			// TempTerrainDataRHIRef->Release();
			// TempTerrainDataUAVRef->Release();
			//
			// Data->Mutex.Unlock();
			Data->bAvailable = true;
		});
}
