﻿// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainMaths/Modules/HITerrainSlopeGPU.h"

#include "RenderGraphUtils.h"
IMPLEMENT_GLOBAL_SHADER(FSlopeShader, "/TerrainShaders/SlopeShader.usf", "Main", SF_Compute);

void FHITerrainSlopeGPU::ApplyModule(UHITerrainData* Data)
{
	while(!Data->bAvailable)
	{
		FPlatformProcess::Sleep(0.1);
	}
	Data->bAvailable = false;

	Data->AddChannel("sediment", ETerrainDataType::FLOAT);
	Data->AddChannel("r", ETerrainDataType::FLOAT);

	ENQUEUE_RENDER_COMMAND(HumidityModuleCommand)(
		[=](FRHICommandListImmediate& RHICmdList)
		{
			
			int32 Size = Data->RealSize();
			
			TResourceArray<float> TerrainSlopeBuffer;
			
			for(int32 i = 0; i < Size; i++)
			{
				for(int32 j = 0; j < Size; j++)
				{
					TerrainSlopeBuffer.Add(Data->GetChannel("height")->GetFloat(i, j));
					TerrainSlopeBuffer.Add(Data->GetChannel("sediment")->GetFloat(i, j));
					TerrainSlopeBuffer.Add(0.0f);
				}
			}
			TerrainSlopeBuffer.SetAllowCPUAccess(true);
			
			FRHIResourceCreateInfo TerrainSlopeCreateInfo;
			TerrainSlopeCreateInfo.ResourceArray = &TerrainSlopeBuffer;
			
			FStructuredBufferRHIRef TerrainSlopeRHIRef = RHICreateStructuredBuffer(sizeof(float), sizeof(float) * Size * Size * 3, BUF_UnorderedAccess | BUF_ShaderResource, TerrainSlopeCreateInfo);
			FUnorderedAccessViewRHIRef TerrainSlopeUAVRef = RHICreateUnorderedAccessView(TerrainSlopeRHIRef, true, false);

			TShaderMapRef<FSlopeShader> SlopeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			FSlopeShader::FParameters Parameters;
			Parameters.Size = Size;
			Parameters.TerrainSlopeData = TerrainSlopeUAVRef;
			
			FComputeShaderUtils::Dispatch(RHICmdList, SlopeShader, Parameters, FIntVector(Size / 8, Size / 8, 1));

			GDynamicRHI->RHIBlockUntilGPUIdle();
			
			float* TerrainSlopeDataSrc = (float*)RHICmdList.LockStructuredBuffer(TerrainSlopeRHIRef.GetReference(), 0, sizeof(float) * Size * Size * 3, EResourceLockMode::RLM_ReadOnly);

			TArray<float> ResultSlopeData;
			ResultSlopeData.Reserve(Size * Size * 3);
			ResultSlopeData.AddUninitialized(Size * Size * 3);

			FMemory::Memcpy(ResultSlopeData.GetData(), TerrainSlopeDataSrc, sizeof(float) * Size * Size * 3);
			
			RHICmdList.UnlockStructuredBuffer(TerrainSlopeRHIRef.GetReference());

			for(int32 i = 0; i < Size; i++)
			{
				for(int32 j = 0; j < Size; j++)
				{
					int32 Index = (i * Size + j) * 3;
					float Slope = ResultSlopeData[Index + 2];
					Slope = 1.0f - FMath::Clamp(Slope * 10, 0.0f, 1.0f);
					Data->SetChannelValue("r", i, j, Slope);
				}
			}
			TerrainSlopeRHIRef.SafeRelease();
			TerrainSlopeUAVRef.SafeRelease();
			
			// Data->Mutex.Unlock();
			Data->bAvailable = true;
		});
}