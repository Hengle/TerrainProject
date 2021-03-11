// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainAlgorithms/MountainAlgorithm.h"


#include "TerrainDatas/HITerrainData.h"

void UMountainAlgorithm::SetMountainData(int32 InSeed, float InMountainHeight, float InMountainScale)
{
	MountainHeight = InMountainHeight;
	MountainScale = InMountainScale;
	Seed = InSeed;
	MountainGenerator.Init(Seed);
	bIsInited = true;
}

void UMountainAlgorithm::Apply(UHITerrainData* Data)
{
	Super::Apply(Data);
	int32 Size = Data->Size();
	for(int32 i = 0; i < Size; i++)
	{
		for(int32 j = 0; j < Size; j++)
		{
			float LocationX = (float)i / Data->GetChunkSize();
			float LocationY = (float)j / Data->GetChunkSize();
			float MountainValue = MountainGenerator.GetValue(LocationX * MountainScale, LocationY * MountainScale) * MountainHeight;
			Data->SetSample(i, j, MountainValue);
		}
	}
}
