﻿// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainMaths/Modules/HITerrainHumidityGPU.h"
#include "RenderGraphUtils.h"
#include "RenderTargetPool.h"

IMPLEMENT_GLOBAL_SHADER(FHumidityShader, "/TerrainShaders/HumidityShader.usf", "Main", SF_Compute);

FHITerrainHumidityGPU::FHITerrainHumidityGPU(): NumIteration(10)
{
}

void FHITerrainHumidityGPU::SetNumIteration(int32 InNumIteration)
{
	NumIteration = InNumIteration;
}

void FHITerrainHumidityGPU::ApplyModule(UHITerrainData* Data)
{
	while(!Data->bAvailable)
	{
		FPlatformProcess::Sleep(0.1);
	}
	Data->bAvailable = false;

	Data->AddChannel("water", ETerrainDataType::FLOAT);
	Data->AddChannel("b", ETerrainDataType::FLOAT);

	ENQUEUE_RENDER_COMMAND(HumidityModuleCommand)(
		[=](FRHICommandListImmediate& RHICmdList)
		{
			
			int32 Size = Data->RealSize();
			
			TResourceArray<float> WaterHumidityBuffer;
			
			for(int32 i = 0; i < Size; i++)
			{
				for(int32 j = 0; j < Size; j++)
				{
					WaterHumidityBuffer.Add(Data->GetChannel("water")->GetFloat(i, j));
					WaterHumidityBuffer.Add(0.0f);
				}
			}
			WaterHumidityBuffer.SetAllowCPUAccess(true);
			
			FRHIResourceCreateInfo WaterHumidityCreateInfo;
			WaterHumidityCreateInfo.ResourceArray = &WaterHumidityBuffer;
			
			FStructuredBufferRHIRef WaterHumidityRHIRef = RHICreateStructuredBuffer(sizeof(float), sizeof(float) * Size * Size * 2, BUF_UnorderedAccess | BUF_ShaderResource, WaterHumidityCreateInfo);
			FUnorderedAccessViewRHIRef WaterHumidityUAVRef = RHICreateUnorderedAccessView(WaterHumidityRHIRef, true, false);

			TShaderMapRef<FHumidityShader> HumidityShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			FHumidityShader::FParameters Parameters;
			Parameters.Size = Size;
			Parameters.Step = 1.0f / NumIteration;
			Parameters.WaterHumidityData = WaterHumidityUAVRef;
			for(int32 i = 0; i < NumIteration; i++)
			{
				FComputeShaderUtils::Dispatch(RHICmdList, HumidityShader, Parameters, FIntVector(Size / 8, Size / 8, 1));
			}

			GDynamicRHI->RHIBlockUntilGPUIdle();
			
			float* WaterHumidityDataSrc = (float*)RHICmdList.LockStructuredBuffer(WaterHumidityRHIRef.GetReference(), 0, sizeof(float) * Size * Size * 2, EResourceLockMode::RLM_ReadOnly);

			TArray<float> ResultHumidityData;
			ResultHumidityData.Reserve(Size * Size * 2);
			ResultHumidityData.AddUninitialized(Size * Size * 2);

			FMemory::Memcpy(ResultHumidityData.GetData(), WaterHumidityDataSrc, sizeof(float) * Size * Size * 2);
			
			RHICmdList.UnlockStructuredBuffer(WaterHumidityRHIRef.GetReference());

			for(int32 i = 0; i < Size; i++)
			{
				for(int32 j = 0; j < Size; j++)
				{
					int32 Index = (i * Size + j) * 2;
					float Humidity = ResultHumidityData[Index + 1];
					// if(Humidity != 0.0f)
					// {
					// 	// UE_LOG(LogHITerrain, Log, TEXT("HI"))
					// }
					// Humidity = FMath::Clamp(Humidity / 100, 0.0f, 1.0f);
					Data->SetChannelValue("b", i, j, Humidity);
				}
			}
			WaterHumidityRHIRef.SafeRelease();
			WaterHumidityUAVRef.SafeRelease();
			
			// Data->Mutex.Unlock();
			Data->bAvailable = true;
		});
}