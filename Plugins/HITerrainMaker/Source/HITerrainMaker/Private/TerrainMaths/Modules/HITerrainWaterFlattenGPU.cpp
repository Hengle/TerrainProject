// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainMaths/Modules/HITerrainWaterFlattenGPU.h"
#include "Async/Async.h"
#include "RenderGraphUtils.h"

IMPLEMENT_GLOBAL_SHADER(FWaterFlattenShader, "/TerrainShaders/WaterFlattenShader.usf", "Main", SF_Compute);

FHITerrainWaterFlattenGPU::FHITerrainWaterFlattenGPU():NumIteration(500), DeltaTime(0.02)
{
}

void FHITerrainWaterFlattenGPU::SetNumIteration(int32 InNumIteration)
{
	NumIteration = InNumIteration;
}

void FHITerrainWaterFlattenGPU::SetDeltaTime(float InDeltaTime)
{
	DeltaTime = InDeltaTime;
}

void FHITerrainWaterFlattenGPU::ApplyModule(UHITerrainData* Data)
{
	Data->AddChannel("water", ETerrainDataType::FLOAT);
	Data->AddChannel("sediment", ETerrainDataType::FLOAT);
	ApplyWaterFlattenShader(Data);
}

void FHITerrainWaterFlattenGPU::ApplyWaterFlattenShader(UHITerrainData* Data)
{
	FRHICommandListImmediate& RHICmdList = GRHICommandList.GetImmediateCommandList();
	
	
	ENQUEUE_RENDER_COMMAND(WaterFlattenModuleCommand)(
		[=](FRHICommandListImmediate& RHICmdList)
		{
			
			TShaderMapRef<FWaterFlattenShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			int32 Size = Data->Size();
		
			TResourceArray<float> TerrainDataBuffer;
			TResourceArray<float> FluxBuffer;

			TResourceArray<float> TempTerrainDataBuffer;
			TResourceArray<float> TempFluxBuffer;
			
			
			for(int32 i = 0; i < Size; i++)
			{
				for(int32 j = 0; j < Size; j++)
				{
					TerrainDataBuffer.Add(Data->GetChannel("height")->GetFloat(i, j));
					TerrainDataBuffer.Add(Data->GetChannel("water")->GetFloat(i, j));
					TerrainDataBuffer.Add(Data->GetChannel("sediment")->GetFloat(i, j));

					FluxBuffer.Add(0.0f);
					FluxBuffer.Add(0.0f);
					FluxBuffer.Add(0.0f);
					FluxBuffer.Add(0.0f);

					TempFluxBuffer.Add(0.0f);
					TempFluxBuffer.Add(0.0f);
					TempFluxBuffer.Add(0.0f);
					TempFluxBuffer.Add(0.0f);

					TempTerrainDataBuffer.Add(Data->GetChannel("height")->GetFloat(i, j));
					TempTerrainDataBuffer.Add(Data->GetChannel("water")->GetFloat(i, j));
					TempTerrainDataBuffer.Add(Data->GetChannel("sediment")->GetFloat(i, j));
				}
			}
			TerrainDataBuffer.SetAllowCPUAccess(true);
			
			FRHIResourceCreateInfo TerrainDataCreateInfo;
			TerrainDataCreateInfo.ResourceArray = &TerrainDataBuffer;
			FRHIResourceCreateInfo FluxCreateInfo;
			FluxCreateInfo.ResourceArray = &FluxBuffer;

			FRHIResourceCreateInfo TempTerrainDataCreateInfo;
			TempTerrainDataCreateInfo.ResourceArray = &TempTerrainDataBuffer;
			FRHIResourceCreateInfo TempFluxCreateInfo;
			TempFluxCreateInfo.ResourceArray = &TempFluxBuffer;
			
			FStructuredBufferRHIRef TerrainDataRHIRef = RHICreateStructuredBuffer(sizeof(float), sizeof(float) * Size * Size * 3, BUF_UnorderedAccess | BUF_ShaderResource, TerrainDataCreateInfo);
			FUnorderedAccessViewRHIRef TerrainDataUAVRef = RHICreateUnorderedAccessView(TerrainDataRHIRef, true, false);
			FStructuredBufferRHIRef FluxRHIRef = RHICreateStructuredBuffer(sizeof(float), sizeof(float) * Size * Size * 4, BUF_UnorderedAccess | BUF_ShaderResource, FluxCreateInfo);
			FUnorderedAccessViewRHIRef FluxUAVRef = RHICreateUnorderedAccessView(FluxRHIRef, true, false);
			
			FStructuredBufferRHIRef TempTerrainDataRHIRef = RHICreateStructuredBuffer(sizeof(float), sizeof(float) * Size * Size * 3, BUF_UnorderedAccess | BUF_ShaderResource, TempTerrainDataCreateInfo);
			FUnorderedAccessViewRHIRef TempTerrainDataUAVRef = RHICreateUnorderedAccessView(TempTerrainDataRHIRef, true, false);
			FStructuredBufferRHIRef TempFluxRHIRef = RHICreateStructuredBuffer(sizeof(float), sizeof(float) * Size * Size * 4, BUF_UnorderedAccess | BUF_ShaderResource, TempFluxCreateInfo);
			FUnorderedAccessViewRHIRef TempFluxUAVRef = RHICreateUnorderedAccessView(TempFluxRHIRef, true, false);
			
			FWaterFlattenShader::FParameters Parameters;
			Parameters.Size = Size;
			Parameters.NumIteration = NumIteration;
			Parameters.DeltaTime = DeltaTime;
			Parameters.TerrainData = TerrainDataUAVRef;
			Parameters.Flux = FluxUAVRef;
			
			Parameters.TempTerrainData = TempTerrainDataUAVRef;
			Parameters.TempFlux = TempFluxUAVRef;


			for(int32 i = 0; i < NumIteration; i++)
			{
				FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, Parameters, FIntVector(Size / 1, Size / 1, 1));
						
					AsyncTask(ENamedThreads::GameThread, []()
					{
						FRenderCommandFence Fence;
						Fence.BeginFence();
						Fence.Wait();
					});
			}
			float* TerrainDataSrc = (float*)RHICmdList.LockStructuredBuffer(TerrainDataRHIRef.GetReference(), 0, sizeof(float) * Size * Size * 3, EResourceLockMode::RLM_ReadOnly);
		
			TArray<float> ResultTerrainData;
			ResultTerrainData.Reserve(Size * Size * 3);
			ResultTerrainData.AddUninitialized(Size * Size * 3);
			
			FMemory::Memcpy(ResultTerrainData.GetData(), TerrainDataSrc, sizeof(float) * Size * Size * 3);
			
			RHICmdList.UnlockStructuredBuffer(TerrainDataRHIRef.GetReference());
			
			for(int32 i = 0; i < Size; i++)
			{
				for(int32 j = 0; j < Size; j++)
				{
					int32 Index = (i * Size + j) * 3;
					// Data->SetChannelValue("height", i, j, ResultTerrainData[Index]);
					Data->SetChannelValue("water", i, j, ResultTerrainData[Index + 1]);
				}
			}
		});

}
