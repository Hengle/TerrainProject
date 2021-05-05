// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainMaths/Modules/HITerrainErosionGPU.h"

#include "RenderGraphUtils.h"
#include "RenderTargetPool.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Rendering/Texture2DResource.h"

IMPLEMENT_GLOBAL_SHADER(FErosionShader, "/TerrainShaders/TestComputeShader.usf", "MainComputeShader", SF_Compute);



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
	for(int32 i = 0; i < Size; i++)
	{
		for(int32 j = 0; j < Size; j++)
		{
			HeightBuffer.Add(Data->GetChannel("height")->GetFloat(i, j));
			WaterBuffer.Add(Data->GetChannel("water")->GetFloat(i, j));
			SedimentBuffer.Add(Data->GetChannel("sediment")->GetFloat(i, j));
			HardnessBuffer.Add(Data->GetChannel("hardness")->GetFloat(i, j));
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
	
	FStructuredBufferRHIRef HeightRHIRef = RHICreateStructuredBuffer(sizeof(float), sizeof(float) * Size * Size, BUF_UnorderedAccess | BUF_ShaderResource, HeightCreateInfo);
	FUnorderedAccessViewRHIRef HeightUAVRef = RHICreateUnorderedAccessView(HeightRHIRef, true, false);
	FStructuredBufferRHIRef WaterRHIRef = RHICreateStructuredBuffer(sizeof(float), sizeof(float) * Size * Size, BUF_UnorderedAccess | BUF_ShaderResource, WaterCreateInfo);
	FUnorderedAccessViewRHIRef WaterUAVRef = RHICreateUnorderedAccessView(WaterRHIRef, true, false);
	FStructuredBufferRHIRef SedimentRHIRef = RHICreateStructuredBuffer(sizeof(float), sizeof(float) * Size * Size, BUF_UnorderedAccess | BUF_ShaderResource, SedimentCreateInfo);
	FUnorderedAccessViewRHIRef SedimentUAVRef = RHICreateUnorderedAccessView(SedimentRHIRef, true, false);
	FStructuredBufferRHIRef HardnessRHIRef = RHICreateStructuredBuffer(sizeof(float), sizeof(float) * Size * Size, BUF_UnorderedAccess | BUF_ShaderResource, HardnessCreateInfo);
	FUnorderedAccessViewRHIRef HardnessUAVRef = RHICreateUnorderedAccessView(HardnessRHIRef, true, false);

	FErosionShader::FParameters Parameters;
	Parameters.Size = Size;
	Parameters.TerrainHeight = HeightUAVRef;
	Parameters.TerrainWater = WaterUAVRef;
	Parameters.TerrainSediment = SedimentUAVRef;
	Parameters.TerrainHardness = HardnessUAVRef;

	TShaderMapRef<FErosionShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, Parameters, 
						FIntVector(FMath::DivideAndRoundUp(Size, 32),
								FMath::DivideAndRoundUp(Size, 32), 1));
	
	ENQUEUE_RENDER_COMMAND(ErosionModuleCommand)(
		[=](FRHICommandListImmediate& RHICmdList){
			

			float* HeightSrc = (float*)RHICmdList.LockStructuredBuffer(HeightRHIRef.GetReference(), 0, sizeof(float) * Size * Size, EResourceLockMode::RLM_ReadOnly);
			float* WaterSrc = (float*)RHICmdList.LockStructuredBuffer(WaterRHIRef.GetReference(), 0, sizeof(float) * Size * Size, EResourceLockMode::RLM_ReadOnly);
			float* SedimentSrc = (float*)RHICmdList.LockStructuredBuffer(SedimentRHIRef.GetReference(), 0, sizeof(float) * Size * Size, EResourceLockMode::RLM_ReadOnly);
			float* HardnessSrc = (float*)RHICmdList.LockStructuredBuffer(HardnessRHIRef.GetReference(), 0, sizeof(float) * Size * Size, EResourceLockMode::RLM_ReadOnly);

			TArray<float> ResultHeight;
			TArray<float> ResultWater;
			TArray<float> ResultSediment;
			TArray<float> ResultHardness;

			for(int32 i = 0; i < Size; i++)
			{
				for(int32 j = 0; j < Size; j++)
				{
					ResultHeight.Add(*HeightSrc);
					ResultWater.Add(*WaterSrc);
					ResultSediment.Add(*SedimentSrc);
					ResultHardness.Add(*HardnessSrc);
					HeightSrc++;
					WaterSrc++;
					SedimentSrc++;
					HardnessSrc++;
				}
			}
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
