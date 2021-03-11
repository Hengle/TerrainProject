#include "HITerrainInstance.h"
#include "Providers/SampleTerrainProvider.h"
#include "HITerrainActor.h"
#include "TerrainDatas/HITerrainData.h"
#include "HITerrainManager.h"
#include "TerrainAlgorithms/MountainAlgorithm.h"
#include "TerrainAlgorithms/PlainAlgorithm.h"

AHITerrainInstance::AHITerrainInstance() 
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void AHITerrainInstance::Init(const FTerrainInformation& InTerrainInformation) 
{
	TerrainInformation = InTerrainInformation;
	InitAlgorithms();
	Data = NewObject<UHITerrainData>(this);
	Data->SetChunkNums(10);
	Data->SetChunkSize(TerrainInformation.ChunkSize / 100);
	Data->SetAlgorithms(Algorithms);
	Data->OnDataGenerated.BindUObject(this, &AHITerrainInstance::OnDataGenerated);
	FRunnableThread::Create(Data, TEXT("HITerrainData"));
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &AHITerrainInstance::ProcessQueue, ProcessQueueInterval, true, 0.0f);
}

void AHITerrainInstance::ProcessQueue() 
{
	if (!CreateChunkQueue.IsEmpty()) 
	{
		TPair<int32, int32> Index;
		bool bCreated = false;
		while (!bCreated)
		{
			bool bSuccess = CreateChunkQueue.Dequeue(Index);
			if (bSuccess)
			{
				bCreated = CreateThunk(Index);
			}
			else
			{
				break;
			}
		}
	}
}

bool AHITerrainInstance::CreateThunk(TPair<int32, int32> Index)
{
	if (Chunks.Contains(Index)) 
	{
		AHITerrainActor* TerrainActor = Chunks[Index];
		TerrainActor->Size = ChunkSize / 100;
		TerrainActor->Step = 100;
		TerrainActor->Material = Material;
		TerrainActor->Initialize(Data, Index);
		UE_LOG(LogHITerrain, Log, TEXT("HITerrainInstance: Create Chunk[%d, %d]"), Index.Key, Index.Value)
		return true;
	}
	else 
	{
		return false;
	}
}

void AHITerrainInstance::OnDataGenerated() 
{
	PrimaryActorTick.SetTickFunctionEnable(true);
}

void AHITerrainInstance::Tick(float DeltaTime) 
{
	Super::Tick(DeltaTime);
	TickChunks();
}

void AHITerrainInstance::InitAlgorithms()
{
	if(TerrainInformation.TerrainType == ETerrainType::SAMPLE)
	{
		UMountainAlgorithm* MountainAlgorithm = NewObject<UMountainAlgorithm>(this);
		MountainAlgorithm->SetMountainData(TerrainInformation.Seed, TerrainInformation.MountainHeight, TerrainInformation.MountainScale);
		Algorithms.Add(MountainAlgorithm);
		UPlainAlgorithm* PlainAlgorithm = NewObject<UPlainAlgorithm>(this);
		PlainAlgorithm->SetPlainData(TerrainInformation.Seed, TerrainInformation.PlainHeight, TerrainInformation.PlainScale, TerrainInformation.PlainThreshold);
		Algorithms.Add(PlainAlgorithm);
	}
}

void AHITerrainInstance::TickChunks()
{
	FVector PlayerLocation = UHITerrainManager::Get()->GetPlayerLocation(GetWorld());
	FVector PlayerOffset = PlayerLocation - GetActorLocation();
	int32 xStart = FMath::Floor((PlayerOffset.X - RenderDistance) / ChunkSize);
	int32 xEnd = FMath::Floor((PlayerOffset.X + RenderDistance) / ChunkSize);
	int32 yStart = FMath::Floor((PlayerOffset.Y - RenderDistance) / ChunkSize);
	int32 yEnd = FMath::Floor((PlayerOffset.Y + RenderDistance) / ChunkSize);
	for (int32 x = xStart; x < xEnd; x++) {
		for (int32 y = yStart; y < yEnd; y++) {
			TPair<int32, int32> Index(x, y);
			if (x < 0 || x >= TerrainInformation.ChunkNum || y < 0 || y >= TerrainInformation.ChunkNum)
			{
				continue;
			}
			if (Chunks.Contains(Index)) 
			{
				
			}
			else 
			{
				UE_LOG(LogHITerrain, Log, TEXT("HITerrainInstance: Need ProceduralMesh[%d, %d]"), Index.Key, Index.Value)
				AHITerrainActor* TerrainActor = Cast<AHITerrainActor>(GetWorld()->SpawnActor(AHITerrainActor::StaticClass(), &TerrainInformation.Position));
				Chunks.Add(Index, TerrainActor);
				CreateChunkQueue.Enqueue(Index);
			}
		}
	}
}
