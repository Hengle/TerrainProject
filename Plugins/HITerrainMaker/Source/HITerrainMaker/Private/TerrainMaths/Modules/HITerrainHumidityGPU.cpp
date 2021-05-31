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
			WaterHumidityCreateInfo.DebugName = TEXT("WaterHumidity");
			WaterHumidity.Initialize(sizeof(float), Size * Size * 2, WaterHumidityCreateInfo);
			
			TShaderMapRef<FHumidityShader> HumidityShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			FHumidityShader::FParameters Parameters;
			Parameters.Size = Size;
			Parameters.Step = 1.0f / NumIteration;
			Parameters.WaterHumidityData = WaterHumidity.UAV;
			
			for(int32 i = 0; i < NumIteration; i++)
			{
				FComputeShaderUtils::Dispatch(RHICmdList, HumidityShader, Parameters, FIntVector(Size / 8, Size / 8, 1));
			}
			
			float* WaterHumidityDataSrc = (float*)RHICmdList.LockStructuredBuffer(WaterHumidity.Buffer.GetReference(), 0, sizeof(float) * Size * Size * 2, EResourceLockMode::RLM_ReadOnly);

			TArray<float> ResultHumidityData;
			ResultHumidityData.Reserve(Size * Size * 2);
			ResultHumidityData.AddUninitialized(Size * Size * 2);

			FMemory::Memcpy(ResultHumidityData.GetData(), WaterHumidityDataSrc, sizeof(float) * Size * Size * 2);
			
			RHICmdList.UnlockStructuredBuffer(WaterHumidity.Buffer.GetReference());

			for(int32 i = 0; i < Size; i++)
			{
				for(int32 j = 0; j < Size; j++)
				{
					int32 Index = (i * Size + j) * 2;
					float Humidity = ResultHumidityData[Index + 1];
					Data->SetChannelValue("b", i, j, Humidity);
				}
			}

			WaterHumidity.Release();
			Data->bAvailable = true;
		});
}
