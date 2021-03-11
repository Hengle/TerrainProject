#include "TerrainAlgorithms/PlainAlgorithm.h"
#include "TerrainDatas/HITerrainData.h"

void UPlainAlgorithm::SetPlainData(int32 InSeed, float InPlainHeight, float InPlainScale, float InPlainThreshold)
{
	PlainHeight = InPlainHeight;
	PlainScale = InPlainScale;
	PlainThreshold = InPlainThreshold;
	Seed = InSeed;
	PlainGenerator.Init(Seed);
	bIsInited = true;
}

void UPlainAlgorithm::Apply(UHITerrainData* Data)
{
	Super::Apply(Data);
	int32 Size = Data->Size();
	for(int32 i = 0; i < Size; i++)
	{
		for(int32 j = 0; j < Size; j++)
		{
			float LocationX = (float)i / Data->GetChunkSize();
			float LocationY = (float)j / Data->GetChunkSize();
			float Value = Data->GetSample(i, j);
			if(Value < PlainThreshold)
			{
				Value = PlainThreshold;
			}
			float PlainValue = PlainGenerator.GetValue(LocationX * PlainScale, LocationY * PlainScale) * PlainHeight;
			Value += PlainValue;
			Data->SetSample(i, j, Value);
		}
	}
}