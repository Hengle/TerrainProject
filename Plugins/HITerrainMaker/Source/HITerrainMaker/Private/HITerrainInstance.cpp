#include "HITerrainInstance.h"
#include "HITerrainActor.h"
#include "TerrainDatas/HITerrainData.h"
#include "HITerrainManager.h"
#include "TerrainAlgorithms/EcoSystemAlgorithm.h"
#include "TerrainAlgorithms/SampleAlgorithms/ContinentDefinitionAlgorithm.h"
#include "TerrainAlgorithms/SampleAlgorithms/FinalPlanetAlgorithm.h"
#include "TerrainAlgorithms/SampleAlgorithms/SampleAlgorithm.h"
#include "TerrainAlgorithms/SampleAlgorithms/ScaledMountainousTerrainAlgorithm.h"
#include "TerrainComponents/HITerrainChunkTicker.h"

FTerrainInformationPtr AHITerrainInstance::GetTerrainInformation() const
{
	return TerrainInformation;
}

bool AHITerrainInstance::ContainsChunk(TPair<int32, int32> Index) const
{
	return Chunks.Contains(Index);
}

bool AHITerrainInstance::IsChunkGenerated(TPair<int32, int32> Index)
{
	if(ContainsChunk(Index))
	{
		return Chunks[Index]->IsGenerated();
	}
	return false;
}

void AHITerrainInstance::AddChunk(TPair<int32, int32> Index)
{
	if(!Chunks.Contains(Index))
	{
		AHITerrainActor* TerrainActor = Cast<AHITerrainActor>(GetWorld()->SpawnActor(AHITerrainActor::StaticClass(), &TerrainInformation->Position));
        Chunks.Add(Index, TerrainActor);
		TerrainActor->Size = TerrainInformation->ChunkSize / 100;
		TerrainActor->Step = 100;
		
		TerrainActor->Initialize(Data, TerrainInformation, Index);
	}
}

void AHITerrainInstance::DeleteChunkNotInSet(const TSet<TPair<int32, int32>>& UpdateSet)
{
	TSet<TPair<int32, int32>> DeleteSet;
	for(TPair<TPair<int32, int32>, AHITerrainActor*> Pair: Chunks)
	{
		TPair<int32, int32> Index = Pair.Key;
		if(!UpdateSet.Contains(Index) && Chunks[Index]->IsGenerated())
		{
			DeleteSet.Add(Index);
		}
	}
	for(TPair<int32, int32> Index: DeleteSet)
	{
		Chunks[Index]->DeleteChunk();
		UE_LOG(LogHITerrain, Log, TEXT("AHITerrainInstance::DeleteChunkNotInSet [%d, %d]"), Index.Key, Index.Value)
	}
}

bool AHITerrainInstance::GenerateChunkTerrain(TPair<int32, int32> Index)
{
	if(Chunks.Contains(Index))
	{
		AHITerrainActor* TerrainActor = Chunks[Index];
		TPair<int32, int32> PlayerIndex = GetPlayerPositionIndex();
		int32 OffSetX = FMath::Abs(PlayerIndex.Key - Index.Key);
		int32 OffSetY = FMath::Abs(PlayerIndex.Value - Index.Value);
		if(OffSetX > TerrainInformation->RenderDistance || OffSetY > TerrainInformation->RenderDistance)
		{
			return false;
		}
		else if(!TerrainActor->IsGenerated())
		{
			TerrainActor->Material = Material;
			TerrainActor->GenerateChunk();
			UE_LOG(LogHITerrain, Log, TEXT("AHITerrainInstance::GenerateChunkTerrain [%d, %d]"), Index.Key, Index.Value)
			return true;
		}
	}
	return false;
}

bool AHITerrainInstance::UpdateChunk(TPair<int32, int32> Index)
{
	return false;
}

TPair<int32, int32> AHITerrainInstance::GetPlayerPositionIndex()
{
	FVector PlayerLocation = UHITerrainManager::Get()->GetPlayerLocation(GetWorld());
	FVector PlayerOffset = PlayerLocation - GetActorLocation();
	return TPair<int32, int32>(PlayerOffset.X / TerrainInformation->ChunkSize, PlayerOffset.Y / TerrainInformation->ChunkSize);
}

AHITerrainInstance::AHITerrainInstance() 
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void AHITerrainInstance::Init(FTerrainInformationPtr InTerrainInformation) 
{
	TerrainInformation = InTerrainInformation;
	InitAlgorithms();
	Data = NewObject<UHITerrainData>(this);
	Data->SetChunkNums(TerrainInformation->ChunkNum);
	Data->SetChunkSize(TerrainInformation->ChunkSize / 100);
	Data->SetAlgorithms(Algorithms);
	Data->SetInformation(TerrainInformation);
	Data->OnDataGenerated.BindUObject(this, &AHITerrainInstance::OnDataGenerated);
	FRunnableThread::Create(Data, TEXT("HITerrainData"));
	ChunkTicker = NewObject<UHITerrainChunkTicker>(this);
	if(ChunkTicker)
	{
		ChunkTicker->RegisterComponent();
	}
	for(int32 i = 0; i < TerrainInformation->ChunkNum; i++)
	{
		for(int32 j = 0; j < TerrainInformation->ChunkNum; j++)
		{
			AddChunk(TPair<int32, int32>(i, j));
		}
	}
}

void AHITerrainInstance::InitAlgorithms()
{
	if(TerrainInformation->TerrainType == ETerrainType::SAMPLE)
	{
		//UFinalPlanetAlgorithm* Algorithm = NewObject<UFinalPlanetAlgorithm>(this);
		UEcoSystemAlgorithm* Algorithm = NewObject<UEcoSystemAlgorithm>(this);
		Algorithm->Init(TerrainInformation);
		Algorithms.Add(Algorithm);
	}
}

void AHITerrainInstance::OnDataGenerated() 
{
	UE_LOG(LogHITerrain, Log, TEXT("AHITerrainInstance::OnDataGenerated"));
	PrimaryActorTick.SetTickFunctionEnable(true);
}

void AHITerrainInstance::Tick(float DeltaTime) 
{
	Super::Tick(DeltaTime);
	ChunkTicker->TickChunks();
}



