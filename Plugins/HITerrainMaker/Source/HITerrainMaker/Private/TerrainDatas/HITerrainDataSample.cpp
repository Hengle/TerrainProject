#include "HITerrainDataSample.h"

void UHITerrainDataSample::InitData() 
{

}

uint32 UHITerrainDataSample::Run()
{
	for (int32 i = 0; i < ChunkNums; i++) 
	{
		for(int32 j = 0; j < ChunkNums; j++)
		{
			GenerateChunkData(i, j);
		}
	}
	OnDataGenerated.ExecuteIfBound();
	return 0;
}

void UHITerrainDataSample::GenerateChunkData(int32 X, int32 Y) 
{
	UHITerrainDataBase::GenerateChunkData(X, Y);
	FChunkInformationPtr Data = ChunkData[TPair<int32, int32>(X, Y)];
	for (int32 i = 0; i < Data->SampleNums; i++)
	{
		for (int32 j = 0; j < Data->SampleNums; j++)
		{
			// TODO£ºÉú³ÉValue
			float Value = 0.0f;
			Data->SetSample(i, j, Value);
		}
	}
}