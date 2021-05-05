// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainMaths/Modules/HITerrainErosionGPU.h"

#include "RenderGraphUtils.h"
#include "RenderTargetPool.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Rendering/Texture2DResource.h"

IMPLEMENT_GLOBAL_SHADER(FErosionShader, "/TerrainShaders/TestComputeShader.usf", "MainComputeShader", SF_Compute);



void FHITerrainErosionGPU::ApplyModule(UHITerrainData* Data)
{
	FRHICommandListImmediate& RHICmdList = GRHICommandList.GetImmediateCommandList();

	int32 SizeX = Data->GetChannel("height")->GetSizeX();
	int32 SizeY = Data->GetChannel("height")->GetSizeY();

	ENQUEUE_RENDER_COMMAND(SceneDrawCompletion)(
		[SizeX, SizeY, Data](FRHICommandListImmediate& RHICmdList){
			TResourceArray<float> BufferData;
			BufferData.Reset();
			for(int32 i = 0; i < SizeX; i++)
			{
				for(int32 j = 0; j < SizeY; j++)
				{
					BufferData.Add(Data->GetChannel("height")->GetFloat(i, j));
				}
			}
			BufferData.SetAllowCPUAccess(true);
			
			FRHIResourceCreateInfo CreateInfo;
			CreateInfo.ResourceArray = &BufferData;
			FStructuredBufferRHIRef StructuredBufferRHIRef = RHICreateStructuredBuffer(sizeof(float), sizeof(float) * SizeX * SizeY, BUF_UnorderedAccess | BUF_ShaderResource, CreateInfo);
			FUnorderedAccessViewRHIRef UnorderedAccessViewRHIRef = RHICreateUnorderedAccessView(StructuredBufferRHIRef, true, false);

			FErosionShader::FParameters Parameters;
			Parameters.SizeX = SizeX;
			Parameters.SizeY = SizeY;
			Parameters.Height = UnorderedAccessViewRHIRef;

			TShaderMapRef<FErosionShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, Parameters, 
								FIntVector(FMath::DivideAndRoundUp(SizeX, 32),
										FMath::DivideAndRoundUp(SizeY, 32), 1));

			float* srcptr = (float*)RHICmdList.LockStructuredBuffer(StructuredBufferRHIRef.GetReference(), 0, sizeof(float) * SizeX * SizeY, EResourceLockMode::RLM_ReadOnly);
			TArray<float> OutData;
			OutData.Reset();
			// FMemory::Memcpy(OutData.GetData(), srcptr, sizeof(float) * SizeX * SizeY);
			for(int32 i = 0; i < SizeX; i++)
			{
				for(int32 j = 0; j < SizeY; j++)
				{
					OutData.Add(*srcptr);
					srcptr++;
				}
			}
			RHICmdList.UnlockStructuredBuffer(StructuredBufferRHIRef.GetReference());

			for(int32 i = 0; i < SizeX; i++)
			{
				for(int32 j = 0; j < SizeY; j++)
				{
					float Value = OutData[i * SizeX + j];
					Data->SetChannelValue("height", i, j, Value);
				}
			}
		}
	);
	
}
