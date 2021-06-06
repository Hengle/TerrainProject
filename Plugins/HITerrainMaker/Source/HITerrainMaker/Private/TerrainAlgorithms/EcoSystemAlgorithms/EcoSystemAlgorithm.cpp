#include "TerrainAlgorithms/EcoSystemAlgorithms/EcoSystemAlgorithm.h"

void UEcoSystemAlgorithm::Init(FTerrainInformationPtr InInformation)
{
	Super::Init(InInformation);
	GrassPerlin.SetSeed(Information->Seed + 1);
	GrassPerlin.SetAmplitude(1.0f);
	GrassPerlin.SetScale(0.01);
}

void UEcoSystemAlgorithm::ApplyAlgorithm(UHITerrainData* Data)
{
	Super::ApplyAlgorithm(Data);
	Data->AddChannel("r", ETerrainDataType::FLOAT);	// 坡度值
	Data->AddChannel("g", ETerrainDataType::FLOAT);
	Data->AddChannel("b", ETerrainDataType::FLOAT); // 湿度值
	Data->AddChannel("a", ETerrainDataType::FLOAT);
	CalculateUnderWaterTerrain(Data);
	SlopeGPU.ApplyModule(Data);
	HumidityGPU.ApplyModule(Data);
	int32 ChunkNum = Information->ChunkNum;
	for(int32 i = 0; i < ChunkNum; i++)
	{
		for(int32 j = 0; j < ChunkNum; j++)
		{
			TPair<int32, int32> Index(i, j);
			GenerateChunkGrassData(Data, Index);
		}
	}
}

void UEcoSystemAlgorithm::DebugAlgorithm(UHITerrainData* Data)
{
	Super::DebugAlgorithm(Data);
	Data->AddChannel("r", ETerrainDataType::FLOAT);	// 坡度值
	Data->AddChannel("g", ETerrainDataType::FLOAT);
	Data->AddChannel("b", ETerrainDataType::FLOAT); // 湿度值
	Data->AddChannel("a", ETerrainDataType::FLOAT);
	FDateTime Time1 = FDateTime::Now();
	LOCK
	CalculateUnderWaterTerrain(Data);
	UNLOCK
	FDateTime Time2 = FDateTime::Now();
	SlopeGPU.ApplyModule(Data);
	LOCK
	UNLOCK
	FDateTime Time3 = FDateTime::Now();
	HumidityGPU.ApplyModule(Data);
	LOCK
	UNLOCK
	FDateTime Time4 = FDateTime::Now();
	LOCK
	int32 ChunkNum = Information->ChunkNum;
	// int32 ChunkNum = 1;
	for(int32 i = 0; i < ChunkNum; i++)
	{
		for(int32 j = 0; j < ChunkNum; j++)
		{
			TPair<int32, int32> Index(i, j);
			GenerateChunkGrassData(Data, Index);
		}
	}
	UNLOCK
	FDateTime Time5 = FDateTime::Now();
	UE_LOG(LogHITerrain, Warning, TEXT("Water: %s"), *(Time2 - Time1).ToString())
	UE_LOG(LogHITerrain, Warning, TEXT("Slope: %s"), *(Time3 - Time2).ToString())
	UE_LOG(LogHITerrain, Warning, TEXT("Humidity: %s"), *(Time4 - Time3).ToString())
	UE_LOG(LogHITerrain, Warning, TEXT("Foliage: %s"), *(Time5 - Time4).ToString())
}

void UEcoSystemAlgorithm::GenerateChunkGrassData(UHITerrainData* Data, TPair<int32, int32>& Index)
{
	int32 Size = Data->GetChunkSize();
	float Step = 100.0f;
	float RecentX = Step, RecentY = Step;
	FRandomStream RandomStream(Information->Seed + Index.Key * Data->GetChunkNums() + Index.Value);
	for(int32 i = 0; i < Size - 1; i++)
	{
		for(int32 j = 0; j < Size - 1; j++)
		{
			float LocationX = Index.Key * Information->ChunkSize + RecentX;
			LocationX += RandomStream.FRand() * 25;
			float LocationY = Index.Value * Information->ChunkSize + RecentY;
			LocationY += RandomStream.FRand() * 25;
			float SlopeValue = 1.0f;
			float WaterValue = 0.0f;
			float HumidityValue = 0.0f;
			Data->GetChannelValue("r", LocationX, LocationY, SlopeValue);
			Data->GetChannelValue("g", LocationX, LocationY, WaterValue);
			Data->GetChannelValue("b", LocationX, LocationY, HumidityValue);
			float GrassValue = GrassPerlin.GetValue(i, j);
			float GrassAmount = 1.0f - Information->EcoSystem_GrassAmount;
			float TreeAmount = Information->EcoSystem_TreeAmount;
			if(HumidityValue + SlopeValue / 2 > GrassValue && RandomStream.FRand() > GrassAmount && WaterValue < 0.1f)
			{
				float LocationZ = Data->GetHeightValue(LocationX, LocationY);
				FFoliageData FoliageData;
				FoliageData.Location = FVector(LocationX, LocationY, LocationZ);
				FoliageData.Rotation = Data->GetRotatorAtLocation(FoliageData.Location);
				FoliageData.Type = RandomStream.RandRange(1, 5);
				Data->AddChunkFoliage(Index, FoliageData);
			}
			else if(HumidityValue + SlopeValue / 2 > GrassValue && RandomStream.FRand() < TreeAmount && WaterValue < 0.1f)
			{
				float LocationZ = Data->GetHeightValue(LocationX, LocationY);
				FFoliageData FoliageData;
				FoliageData.Location = FVector(LocationX, LocationY, LocationZ);
				FoliageData.Rotation = Data->GetRotatorAtLocation(FoliageData.Location);
				FoliageData.Type = 10 + RandomStream.RandRange(1, 2);
				Data->AddChunkFoliage(Index, FoliageData);
			}
			RecentY += Step;
		}
		RecentX += Step;
		RecentY = Step;
	}
	
}

void UEcoSystemAlgorithm::CalculateSlope(UHITerrainData* Data)
{
	int32 Size = Data->Size();
	for(int32 i = 0; i < Size; i++)
	{
		for(int32 j = 0; j < Size; j++)
		{
			float LValue = i == 0? Data->GetHeightValue(i, j) + Data->GetSedimentValue(i, j):
									Data->GetHeightValue(i - 1, j) + Data->GetSedimentValue(i - 1, j);
			float RValue = i == Size - 1? Data->GetHeightValue(i, j) + Data->GetSedimentValue(i, j):
											Data->GetHeightValue(i + 1, j) + Data->GetSedimentValue(i + 1, j);
			float TValue = j == 0? Data->GetHeightValue(i, j) + Data->GetSedimentValue(i, j):
									Data->GetHeightValue(i, j - 1) + Data->GetSedimentValue(i, j - 1);
			float BValue = j == Size - 1? Data->GetHeightValue(i, j) + Data->GetSedimentValue(i, j):
									Data->GetHeightValue(i, j + 1) + Data->GetSedimentValue(i, j + 1);
			float SlopeValue = ((LValue - RValue) * (LValue - RValue) + (TValue - BValue) * (TValue - BValue)) / 40000.0f;
			SlopeValue = 1.0f - FMath::Clamp(SlopeValue, 0.0f, 1.0f);
			Data->SetChannelValue("r", i, j, SlopeValue);
		}
	}
}

void UEcoSystemAlgorithm::CalculateUnderWaterTerrain(UHITerrainData* Data)
{
	int32 Size = Data->Size();
	for(int32 i = 0; i < Size; i++)
	{
		for(int32 j = 0; j < Size; j++)
		{
			float WaterValue;
			Data->GetChannelValue("water", i, j, WaterValue);
			Data->SetChannelValue("g", i, j, WaterValue / 100);
		}
	}
}

void UEcoSystemAlgorithm::CalculateHumidity(UHITerrainData* Data)
{
	int32 Size = Data->Size();
	int32 Scope = 5;
	for(int32 i = 0; i < Size; i++)
	{
		for(int32 j = 0; j < Size; j++)
		{
			float SumWaterValue = 0.0f;
			int32 SumGrid = 0;
			for(int32 u = i - Scope; u < i + Scope + 1; u++)
			{
				for(int32 v = j - Scope; v < j + Scope + 1; v++)
				{
					if(u >= 0 && u < Size && v >=0 && v < Size)
					{
						float UVWaterValue = 0.0f;
						Data->GetChannelValue("water", i, j, UVWaterValue);
						SumWaterValue += UVWaterValue;
						SumGrid ++;
					}
				}
			}
			Data->SetChannelValue("b", i, j, SumWaterValue / SumGrid);
		}
	}
}
