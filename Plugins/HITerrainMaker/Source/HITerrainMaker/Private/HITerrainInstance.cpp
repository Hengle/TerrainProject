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

void AHITerrainInstance::AddChunk(TPair<int32, int32> Index)
{
	if(!Chunks.Contains(Index))
	{
		AHITerrainActor* TerrainActor = Cast<AHITerrainActor>(GetWorld()->SpawnActor(AHITerrainActor::StaticClass(), &TerrainInformation->Position));
        Chunks.Add(Index, TerrainActor);
	}
}

bool AHITerrainInstance::GenerateChunkTerrain(TPair<int32, int32> Index)
{
	if(Chunks.Contains(Index))
	{
		AHITerrainActor* TerrainActor = Chunks[Index];
		TerrainActor->Size = TerrainInformation->ChunkSize / 100;
		TerrainActor->Step = 100;
		TerrainActor->Material = Material;
		TerrainActor->Initialize(Data, Index);
		UE_LOG(LogHITerrain, Log, TEXT("HITerrainInstance: Create Chunk[%d, %d]"), Index.Key, Index.Value)
		return true;
	}
	return false;
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
	Data->SetChunkNums(10);
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
}



void AHITerrainInstance::OnDataGenerated() 
{
	PrimaryActorTick.SetTickFunctionEnable(true);
}

void AHITerrainInstance::Tick(float DeltaTime) 
{
	Super::Tick(DeltaTime);
	ChunkTicker->TickChunks();
}

void AHITerrainInstance::InitAlgorithms()
{
	if(TerrainInformation->TerrainType == ETerrainType::SAMPLE)
	{
		// UFinalPlanetAlgorithm* Algorithm = NewObject<UFinalPlanetAlgorithm>(this);
		UEcoSystemAlgorithm* Algorithm = NewObject<UEcoSystemAlgorithm>(this);
		Algorithm->Init(TerrainInformation);
		Algorithms.Add(Algorithm);
	}
}

