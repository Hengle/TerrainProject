#include "HITerrainInstance.h"
#include "TerrainDatas/HITerrainDataSample.h"
#include "Providers/SampleTerrainProvider.h"
#include "HITerrainActor.h"
#include "HITerrainManager.h"

AHITerrainInstance::AHITerrainInstance() 
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void AHITerrainInstance::Init(const FTerrainInformation& InTerrainInformation) 
{
	TerrainInformation = InTerrainInformation;
	Data = NewObject<UHITerrainDataSample>(this);
	Data->SetSeed(TerrainInformation.Seed);
	Data->OnDataGenerated.BindUObject(this, &AHITerrainInstance::OnDataGenerated);
	FRunnableThread::Create(Data, TEXT("HITerrainData"));
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &AHITerrainInstance::ProcessQueue, ProcessQueueInterval, true, 0.0f);
}

void AHITerrainInstance::ProcessQueue() 
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

bool AHITerrainInstance::CreateThunk(TPair<int32, int32> Index)
{
	if (Chunks.Contains(Index)) 
	{
		/*UProceduralMeshComponent* ProceduralMesh = Chunks[Index];
		ProceduralMesh->CreateMeshSection_LinearColor(0, Data->GetChunkData(Index).Vertices,
			Data->GetChunkData(Index).Triangles,
			Data->GetChunkData(Index).Normals,
			Data->GetChunkData(Index).UV0,
			Data->GetChunkData(Index).VertexColors,
			Data->GetChunkData(Index).Tangents, true);*/
		AHITerrainActor* TerrainActor = Chunks[Index];
		USampleTerrainProvider* Provider = NewObject<USampleTerrainProvider>(this);
		TerrainActor->Initialize(Provider);
		UE_LOG(LOGHITerrain, Log, TEXT("HITerrainInstance: Create Chunk[%d, %d]"), Index.Key, Index.Value)
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
			if (Chunks.Contains(Index)) 
			{

			}
			else 
			{
				UE_LOG(LOGHITerrain, Log, TEXT("HITerrainInstance: Need ProceduralMesh[%d, %d]"), Index.Key, Index.Value)
				//URuntimeMeshComponentStatic* RuntimeMesh = NewObject<URuntimeMeshComponentStatic>(this, URuntimeMeshComponentStatic::StaticClass());
				//Chunks.Add(Index, RuntimeMesh);
				//RuntimeMesh->RegisterComponent();
				AHITerrainActor* TerrainActor = Cast<AHITerrainActor>(GetWorld()->SpawnActor(AHITerrainActor::StaticClass(), &TerrainInformation.Position));
				Chunks.Add(Index, TerrainActor);
				CreateChunkQueue.Enqueue(Index);
			}
		}
	}
	//for (TPair<TPair<int32, int32>, UProceduralMeshComponent*>& Element : Chunks) 
	//{
	//	TPair<int32, int32> Index = Element.Key;
	//	if (Index.Key < xStart || Index.Key >= xEnd || Index.Value < yStart || Index.Value >= yEnd) {
	//		Element.Value->UnregisterComponent();
	//		Element.Value->DestroyComponent();
	//		Chunks.Remove(Element.Key);
	//	}
	//}
}