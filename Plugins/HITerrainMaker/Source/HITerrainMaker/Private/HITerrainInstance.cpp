#include "HITerrainInstance.h"
#include "HITerrainData.h"
#include "HITerrainManager.h"

AHITerrainInstance::AHITerrainInstance() 
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void AHITerrainInstance::Init(const FTerrainInformation& InTerrainInformation) 
{
	TerrainInformation = InTerrainInformation;
	Data = NewObject<UHITerrainData>(this);
	Data->Seed = TerrainInformation.Seed;
	Data->ChunkNum = TerrainInformation.ChunkNum;
	Data->HeightScale = TerrainInformation.HeightScale;
	Data->PositionScale = TerrainInformation.PositionScale;
	Data->OnDataGenerated.BindUObject(this, &AHITerrainInstance::OnDataGenerated);
	Data->InitData(TerrainInformation.TerrainType, TerrainInformation.TerrainNoiseType);
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
		URuntimeMeshComponentStatic* RuntimeMesh = Chunks[Index];
		RuntimeMesh->CreateSectionFromComponents(0, 0, 0, Data->GetChunkData(Index).Vertices,
			Data->GetChunkData(Index).Triangles,
			Data->GetChunkData(Index).Normals,
			Data->GetChunkData(Index).UV0,
			Data->GetChunkData(Index).VertexColors,
			Data->GetChunkData(Index).Tangents);
		
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
	//UE_LOG(LOGHITerrain, Log, TEXT("HITerrainInstance: xStart %d, xEnd %d, yStart %d, yEnd %d"), xStart, xEnd, yStart, yEnd)
	for (int32 x = xStart; x < xEnd; x++) {
		for (int32 y = yStart; y < yEnd; y++) {
			//if (x < 0 || y < 0) 
			//{
			//	continue;
			//}
			//else if (x >= TerrainInformation.ChunkNum || y >= TerrainInformation.ChunkNum) 
			//{
			//	continue;
			//}
			TPair<int32, int32> Index(x, y);
			if (Chunks.Contains(Index)) 
			{

			}
			else 
			{
				UE_LOG(LOGHITerrain, Log, TEXT("HITerrainInstance: Need ProceduralMesh[%d, %d]"), Index.Key, Index.Value)
				URuntimeMeshComponentStatic* RuntimeMesh = NewObject<URuntimeMeshComponentStatic>(this, URuntimeMeshComponentStatic::StaticClass());
				Chunks.Add(Index, RuntimeMesh);
				RuntimeMesh->RegisterComponent();
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