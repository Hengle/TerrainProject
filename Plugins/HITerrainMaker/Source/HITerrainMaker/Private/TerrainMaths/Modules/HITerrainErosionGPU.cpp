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

	// float** Heights = new float*[SizeX];
	// for(int32 i = 0; i < SizeX; i++)
	// {
	// 	Heights[i] = new float[SizeY];
	// 	for(int32 j = 0; j < SizeY; j++)
	// 	{
	// 		Heights[i][j] = Data->GetChannel("height")->GetFloat(i, j);
	// 	}
	// }
	//
	//
	// FTexture2DRHIRef Texture2DRHIRef = RHIAsyncCreateTexture2D(SizeX, SizeY, PF_A32B32G32R32F, 1, TexCreate_ShaderResource, (void**)Heights, 1);;
	// FUnorderedAccessViewRHIRef UnorderedAccessViewRHIRef = RHICreateUnorderedAccessView(Texture2DRHIRef);
	//
	FErosionShader::FParameters Parameters;
	Parameters.SizeX = SizeX;
	Parameters.SizeY = SizeY;
	Parameters.Height = UnorderedAccessViewRHIRef;
	
	TShaderMapRef<FErosionShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, Parameters, 
								FIntVector(FMath::DivideAndRoundUp(SizeX, 32),
										FMath::DivideAndRoundUp(SizeY, 32), 1));

	
	
	float* srcptr = (float*)RHILockStructuredBuffer(StructuredBufferRHIRef.GetReference(), 0, sizeof(float) * SizeX * SizeY, EResourceLockMode::RLM_ReadOnly);
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
	RHIUnlockStructuredBuffer(StructuredBufferRHIRef.GetReference());
	
	for(int32 i = 0; i < SizeX; i++)
	{
		for(int32 j = 0; j < SizeY; j++)
		{
			float Value = OutData[i * SizeX + j];
			Data->SetChannelValue("height", i, j, Value);
		}
	}
	UE_LOG(LogHITerrain, Warning, TEXT("HI"));
}
