// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainMaths/Modules/HITerrainErosionGPU.h"

#include "RenderGraphUtils.h"
#include "RenderTargetPool.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Rendering/Texture2DResource.h"
#include "Async/Async.h"

IMPLEMENT_GLOBAL_SHADER(FErosionShader, "/TerrainShaders/ErosionShader.usf", "Main", SF_Compute);


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
	for(int32 i = 0; i < Data->Size(); i++)
	{
		for(int32 j = 0; j < Data->Size(); j++)
		{
			Data->SetChannelValue("hardness", i, j, 1.0f);
		}
	}
	ApplyErosionShader(Data);
}

void FHITerrainErosionGPU::ApplyErosionShader(UHITerrainData* Data)
{
	FRHICommandListImmediate& RHICmdList = GRHICommandList.GetImmediateCommandList();
	
	
	ENQUEUE_RENDER_COMMAND(ErosionModuleCommand)(
		[=](FRHICommandListImmediate& RHICmdList)
		{
			int32 Size = Data->Size();
			
			TResourceArray<float> TerrainDataBuffer;
			TResourceArray<float> FluxBuffer;
			TResourceArray<float> TerrainFluxBuffer;
			TResourceArray<float> VelocityBuffer;

			TResourceArray<float> TempFluxBuffer;
			TResourceArray<float> TempTerrainFluxBuffer;
			TResourceArray<float> TempTerrainDataBuffer;

			
			for(int32 i = 0; i < Size; i++)
			{
				for(int32 j = 0; j < Size; j++)
				{
					TerrainDataBuffer.Add(Data->GetChannel("height")->GetFloat(i, j));
					TerrainDataBuffer.Add(Data->GetChannel("water")->GetFloat(i, j));
					TerrainDataBuffer.Add(Data->GetChannel("sediment")->GetFloat(i, j));
					TerrainDataBuffer.Add(Data->GetChannel("hardness")->GetFloat(i, j));
					// TerrainDataBuffer.Add(1.0f);

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

					TempFluxBuffer.Add(0.0f);
					TempFluxBuffer.Add(0.0f);
					TempFluxBuffer.Add(0.0f);
					TempFluxBuffer.Add(0.0f);

					TempTerrainFluxBuffer.Add(0.0f);
					TempTerrainFluxBuffer.Add(0.0f);
					TempTerrainFluxBuffer.Add(0.0f);
					TempTerrainFluxBuffer.Add(0.0f);

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
			FRHIResourceCreateInfo TempFluxCreateInfo;
			TempFluxCreateInfo.ResourceArray = &TempFluxBuffer;
			FRHIResourceCreateInfo TempTerrainFluxCreateInfo;
			TempTerrainFluxCreateInfo.ResourceArray = &TempTerrainFluxBuffer;
			
			FStructuredBufferRHIRef TerrainDataRHIRef = RHICreateStructuredBuffer(sizeof(float), sizeof(float) * Size * Size * 4, BUF_UnorderedAccess | BUF_ShaderResource, TerrainDataCreateInfo);
			FUnorderedAccessViewRHIRef TerrainDataUAVRef = RHICreateUnorderedAccessView(TerrainDataRHIRef, true, false);
			FStructuredBufferRHIRef FluxRHIRef = RHICreateStructuredBuffer(sizeof(float), sizeof(float) * Size * Size * 4, BUF_UnorderedAccess | BUF_ShaderResource, FluxCreateInfo);
			FUnorderedAccessViewRHIRef FluxUAVRef = RHICreateUnorderedAccessView(FluxRHIRef, true, false);
			FStructuredBufferRHIRef TerrainFluxRHIRef = RHICreateStructuredBuffer(sizeof(float), sizeof(float) * Size * Size * 4, BUF_UnorderedAccess | BUF_ShaderResource, TerrainFluxCreateInfo);
			FUnorderedAccessViewRHIRef TerrainFluxUAVRef = RHICreateUnorderedAccessView(TerrainFluxRHIRef, true, false);
			FStructuredBufferRHIRef VelocityRHIRef = RHICreateStructuredBuffer(sizeof(float), sizeof(float) * Size * Size * 3, BUF_UnorderedAccess | BUF_ShaderResource, VelocityCreateInfo);
			FUnorderedAccessViewRHIRef VelocityUAVRef = RHICreateUnorderedAccessView(VelocityRHIRef, true, false);
			
			FStructuredBufferRHIRef TempTerrainDataRHIRef = RHICreateStructuredBuffer(sizeof(float), sizeof(float) * Size * Size * 4, BUF_UnorderedAccess | BUF_ShaderResource, TempTerrainDataCreateInfo);
			FUnorderedAccessViewRHIRef TempTerrainDataUAVRef = RHICreateUnorderedAccessView(TempTerrainDataRHIRef, true, false);
			FStructuredBufferRHIRef TempFluxRHIRef = RHICreateStructuredBuffer(sizeof(float), sizeof(float) * Size * Size * 4, BUF_UnorderedAccess | BUF_ShaderResource, TempFluxCreateInfo);
			FUnorderedAccessViewRHIRef TempFluxUAVRef = RHICreateUnorderedAccessView(TempFluxRHIRef, true, false);
			FStructuredBufferRHIRef TempTerrainFluxRHIRef = RHICreateStructuredBuffer(sizeof(float), sizeof(float) * Size * Size * 4, BUF_UnorderedAccess | BUF_ShaderResource, TempTerrainFluxCreateInfo);
			FUnorderedAccessViewRHIRef TempTerrainFluxUAVRef = RHICreateUnorderedAccessView(TempTerrainFluxRHIRef, true, false);
			
			FErosionShader::FParameters Parameters;
			Parameters.Size = Size;
			Parameters.NumIteration = NumIteration;
			Parameters.DeltaTime = DeltaTime;
			Parameters.GBEnableHydroErosion = bEnableHydroErosion;
			Parameters.GBEnableThermalErosion = bEnableThermalErosion;
			Parameters.HydroErosionScale = HydroErosionScale;
			Parameters.RainAmount = RainAmount;
			Parameters.EvaporationAmount = EvaporationAmount;
			Parameters.ErosionScale = ErosionScale;
			Parameters.DepositionScale = DepositionScale;
			Parameters.SedimentCapacityScale = SedimentCapacityScale;
			Parameters.ThermalErosionScale = ThermalErosionScale;
			Parameters.TerrainData = TerrainDataUAVRef;
			Parameters.Flux = FluxUAVRef;
			Parameters.TerrainFlux = TerrainFluxUAVRef;
			Parameters.Velocity = VelocityUAVRef;
			
			Parameters.TempTerrainData = TempTerrainDataUAVRef;
			Parameters.TempFlux = TempFluxUAVRef;
			Parameters.TempTerrainFlux = TempTerrainFluxUAVRef;

			for(int32 i = 0; i < NumIteration; i++)
			{
				Parameters.CurrentIteration = i;
					TShaderMapRef<FErosionShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

					FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, Parameters, FIntVector(Size / 1, Size / 1, 1));
					AsyncTask(ENamedThreads::GameThread, []()
					{
						FRenderCommandFence Fence;
						Fence.BeginFence();
						Fence.Wait();
					});
			}
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
		});
	// ENQUEUE_RENDER_COMMAND(ErosionModuleCommand1)(
	// 	[=](FRHICommandListImmediate& RHICmdList)
	// 	{
	// 		
	// 		
	// 		
	// 	});
}
