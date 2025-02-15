﻿#include "TerrainMaths/Modules/HITerrainWaterFlattenGPU.h"
#include "RenderGraphUtils.h"

IMPLEMENT_GLOBAL_SHADER(FWaterFlattenShader, "/TerrainShaders/ErosionShader2_CalcFlow.usf", "Main", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FWaterFlattenShader2, "/TerrainShaders/ErosionShader3_ApplyFlow.usf", "Main", SF_Compute);

FHITerrainWaterFlattenGPU::FHITerrainWaterFlattenGPU():NumIteration(1000), DeltaTime(0.02)
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
	while(!Data->bAvailable)
	{
		FPlatformProcess::Sleep(0.1);
	}
	Data->bAvailable = false;
	Data->AddChannel("water", ETerrainDataType::FLOAT);
	Data->AddChannel("sediment", ETerrainDataType::FLOAT);
	ApplyWaterFlattenShader(Data);
}

void FHITerrainWaterFlattenGPU::ApplyWaterFlattenShader(UHITerrainData* Data)
{
	ENQUEUE_RENDER_COMMAND(WaterFlattenModuleCommand)(
		[=](FRHICommandListImmediate& RHICmdList)
		{
			TShaderMapRef<FWaterFlattenShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			TShaderMapRef<FWaterFlattenShader2> ComputeShader2(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			int32 Size = Data->RealSize();
		
			TResourceArray<float> TerrainDataBuffer;
			TResourceArray<float> FluxBuffer;
			
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
				}
			}
			TerrainDataBuffer.SetAllowCPUAccess(true);
			
			FRHIResourceCreateInfo TerrainDataCreateInfo;
			TerrainDataCreateInfo.ResourceArray = &TerrainDataBuffer;
			TerrainDataCreateInfo.DebugName = TEXT("TerrainDataFlatten");
			FRHIResourceCreateInfo FluxCreateInfo;
			FluxCreateInfo.ResourceArray = &FluxBuffer;
			FluxCreateInfo.DebugName = TEXT("FluxFlatten");

			TerrainData.Initialize(sizeof(float), Size * Size * 4, TerrainDataCreateInfo);
			Flux.Initialize(sizeof(float), Size * Size * 4, FluxCreateInfo); 
			
			FWaterFlattenShader::FParameters Parameters;
			Parameters.Size = Size;
			Parameters.DeltaTime = DeltaTime;
			Parameters.TerrainData = TerrainData.UAV;
			Parameters.Flux = Flux.UAV;

			FWaterFlattenShader2::FParameters Parameters2;
			Parameters2.Size = Size;
			Parameters2.DeltaTime = DeltaTime;
			Parameters2.TerrainData = TerrainData.UAV;
			Parameters2.Flux = Flux.UAV;

			

			for(int32 i = 0; i < NumIteration; i++)
			{
				FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, Parameters, FIntVector(Size / 8, Size / 8, 1));
				FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader2, Parameters2, FIntVector(Size / 8, Size / 8, 1));
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
					Data->SetChannelValue("water", i, j, ResultTerrainData[Index + 1]);
				}
			}

			TerrainData.Release();
			Flux.Release();
			
			Data->bAvailable = true;
		});

}
