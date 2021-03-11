#include "HITerrainDataSample.h"

void UHITerrainDataSample::InitData() 
{

}

uint32 UHITerrainDataSample::Run()
{
	//PerlinNoise.Init(Seed);
	MountainGenerator.Init(Seed);
	PlainGenerator.Init(Seed + 1);
	for (int32 i = 0; i < ChunkNums; i++) 
	{
		for(int32 j = 0; j < ChunkNums; j++)
		{
			GenerateChunkData(i, j);
			UE_LOG(LogHITerrain, Log, TEXT("ChunkData[%d, %d] Generated!"), i, j)
		}
	}
	bIsGenerated = true;
	OnDataGenerated.ExecuteIfBound();
	return 0;
}

void UHITerrainDataSample::GenerateChunkData(int32 X, int32 Y) 
{
	UHITerrainDataBase::GenerateChunkData(X, Y);
	FChunkInformationPtr Data = ChunkData[TPair<int32, int32>(X, Y)];
	for (int32 i = 0; i <= Data->SampleNums; i++)
	{
		for (int32 j = 0; j <= Data->SampleNums; j++)
		{
			// TODO：生成Value
			//float Value = 0.0f;
			float LocationX = X + (float)i / Data->SampleNums;
			float LocationY = Y + (float)j / Data->SampleNums;
			/*float Value = PerlinNoise.GetValue(LocationX, LocationY) * Height;*/
			float MountainValue = - MountainGenerator.GetValue(LocationX * MountainScale, LocationY * MountainScale) * MountainHeight;
			float PlainValue = MountainGenerator.GetValue(LocationX * PlainScale, LocationY * PlainScale) * PlainHeight;
			if(MountainValue < PlainThreshold)
			{
				MountainValue = PlainThreshold;
			}
			float Value = MountainValue + PlainValue;
			Data->SetSample(i, j, Value);
		}
	}
}

