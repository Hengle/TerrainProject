// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainMaths/Modules/ThreadSafeTest.h"

void FThreadSafeTest::ApplyModule(UHITerrainData* Data)
{
	while(!Data->bAvailable)
	{
		FPlatformProcess::Sleep(0.1);
	}
	Data->bAvailable = false;
	Data->AddChannel("water", ETerrainDataType::FLOAT);
	int32 Size = Data->Size();
	for(int32 i = 0; i < Size; i++)
	{
		for(int32 j = 0; j < Size; j++)
		{
			// float WaterValue = 0.0f;
			// Data->GetChannelValue("water", i, j, WaterValue);
			Data->SetChannelValue("water", i, j, 1.0f);
		}
	}
	Data->bAvailable = true;
}
