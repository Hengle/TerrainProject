#include "HITerrainInstance.h"
#include "HITerrainActor.h"
#include "TerrainDatas/HITerrainData.h"
#include "HITerrainManager.h"
#include "TerrainAlgorithms/MidtermAlgorithms/PerlinAlgorithm.h"
#include "TerrainAlgorithms/MidtermAlgorithms/RidgedMultiAlgorithm.h"
#include "TerrainAlgorithms/MidtermAlgorithms/VoronoiAlgorithm.h"
#include "TerrainAlgorithms/TestAlgorithms/TestAlgorithm.h"
#include "TerrainAlgorithms/FinalAlgorithm.h"
#include "TerrainAlgorithms/EcoSystemAlgorithms/EcoSystemAlgorithm.h"
#include "TerrainAlgorithms/TestAlgorithms/TestAlgorithm3.h"
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

/*
 * 生成TerrainActor
 */
void AHITerrainInstance::AddChunk(TPair<int32, int32> Index)
{
	if(!Chunks.Contains(Index))
	{
		AHITerrainActor* TerrainActor = Cast<AHITerrainActor>(GetWorld()->SpawnActor(AHITerrainActor::StaticClass(), &TerrainInformation->Position));
        Chunks.Add(Index, TerrainActor);
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
			TerrainActor->WaterMaterial = WaterMaterial;
			TerrainActor->GenerateChunk(GetLODLevel(Index));
			UE_LOG(LogHITerrain, Log, TEXT("AHITerrainInstance::GenerateChunkTerrain [%d, %d]"), Index.Key, Index.Value)
			return true;
		}
	}
	return false;
}

bool AHITerrainInstance::UpdateChunk(TPair<int32, int32> Index)
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
		else
		{
			if(GetLODLevel(Index) != TerrainActor->GetLODLevel())
			{
				TerrainActor->GenerateChunk(GetLODLevel(Index));
				UE_LOG(LogHITerrain, Log, TEXT("AHITerrainInstance::UpdateChunk [%d, %d]"), Index.Key, Index.Value)
				return true;
			}
		}
	}
	return false;
}

TPair<int32, int32> AHITerrainInstance::GetPlayerPositionIndex()
{
	FVector PlayerLocation = UHITerrainManager::Get()->GetPlayerLocation(GetWorld());
	FVector PlayerOffset = PlayerLocation - GetActorLocation();
	return TPair<int32, int32>(PlayerOffset.X / TerrainInformation->ChunkSize, PlayerOffset.Y / TerrainInformation->ChunkSize);
}

ELODLevel AHITerrainInstance::GetLODLevel(TPair<int32, int32> Index)
{
	TPair<int32, int32> PlayerIndex = GetPlayerPositionIndex();
	// 用曼哈顿距离来计算LODLevel
	int32 Distance = FMath::Abs(Index.Key - PlayerIndex.Key) + FMath::Abs(Index.Value - PlayerIndex.Value);
	if(Distance > 6)
	{
		return ELODLevel::LOD_LOW;
	}
	else if(Distance > 2)
	{
		return ELODLevel::LOD_MEDIUM;
	}
	else
	{
		return ELODLevel::LOD_HIGH;
	}
	return ELODLevel::NONE;
}

AHITerrainInstance::AHITerrainInstance() 
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void AHITerrainInstance::Init(FTerrainInformationPtr InTerrainInformation) 
{
	TerrainInformation = InTerrainInformation;
	// 先初始化地形
	InitAlgorithms();
	// 生成TerrainData
	Data = NewObject<UHITerrainData>(this);
	Data->SetChunkNums(TerrainInformation->ChunkNum);
	Data->SetChunkSize(TerrainInformation->ChunkSize / 100);
	Data->SetAlgorithms(Algorithms);
	Data->SetInformation(TerrainInformation);
	Data->OnDataGenerated.BindUObject(this, &AHITerrainInstance::OnDataGenerated);
	FRunnableThread::Create(Data, TEXT("HITerrainData"));
	// 生成ChunkTicker
	ChunkTicker = NewObject<UHITerrainChunkTicker>(this);
	if(ChunkTicker)
	{
		ChunkTicker->RegisterComponent();
	}
	// 最后生成TerrainActor
	for(int32 i = 0; i < TerrainInformation->ChunkNum; i++)
	{
		for(int32 j = 0; j < TerrainInformation->ChunkNum; j++)
		{
			AddChunk(TPair<int32, int32>(i, j));
		}
	}
}

void AHITerrainInstance::Destroyed()
{
	for(auto Iterate: Chunks)
	{
		Iterate.Value->Destroy();
	}
	Super::Destroyed();
}

void AHITerrainInstance::InitAlgorithms()
{
	// 根据类型，初始化地形生成算法
	if(TerrainInformation->TerrainType == ETerrainType::PERLIN)
	{
		UPerlinAlgorithm* Algorithm = NewObject<UPerlinAlgorithm>(this);
		Algorithm->Init(TerrainInformation);
		Algorithms.Add(Algorithm);
	}
	else if (TerrainInformation->TerrainType == ETerrainType::VORONOI)
	{
		UVoronoiAlgorithm* Algorithm = NewObject<UVoronoiAlgorithm>(this);
		Algorithm->Init(TerrainInformation);
		Algorithms.Add(Algorithm);
	}
	else if (TerrainInformation->TerrainType == ETerrainType::RIDGED_MULTI)
	{
		URidgedMultiAlgorithm* Algorithm = NewObject<URidgedMultiAlgorithm>(this);
		Algorithm->Init(TerrainInformation);
		Algorithms.Add(Algorithm);
	}
	else if (TerrainInformation->TerrainType == ETerrainType::TEST)
	{
		UTestAlgorithm* Algorithm = NewObject<UTestAlgorithm>(this);
		Algorithm->Init(TerrainInformation);
		Algorithms.Add(Algorithm);
		
		UEcoSystemAlgorithm* EcoSystemAlgorithm = NewObject<UEcoSystemAlgorithm>(this);
		EcoSystemAlgorithm->Init(TerrainInformation);
		Algorithms.Add(EcoSystemAlgorithm);
		Material = LoadObject<UMaterial>(nullptr, TEXT("/Game/Mat_Basic.Mat_Basic"));
		WaterMaterial = LoadObject<UMaterial>(nullptr, TEXT("/Water/Materials/WaterSurface/Water_Material_CustomMesh.Water_Material_CustomMesh"));
	}
	else if (TerrainInformation->TerrainType == ETerrainType::FINAL)
	{
		UFinalAlgorithm* Algorithm = NewObject<UFinalAlgorithm>(this);
		Algorithm->Init(TerrainInformation);
		Algorithms.Add(Algorithm);
		
		UEcoSystemAlgorithm* EcoSystemAlgorithm = NewObject<UEcoSystemAlgorithm>(this);
		EcoSystemAlgorithm->Init(TerrainInformation);
		Algorithms.Add(EcoSystemAlgorithm);
		Material = LoadObject<UMaterial>(nullptr, TEXT("/Game/Mat_Basic.Mat_Basic"));
		WaterMaterial = LoadObject<UMaterialInstance>(nullptr, TEXT("MaterialInstanceConstant'/Water/Materials/WaterSurface/Water_Material_CustomMesh.Water_Material_CustomMesh'"));
	}
	else if (TerrainInformation->TerrainType == ETerrainType::TEST3)
	{
		UTestAlgorithm3* Algorithm = NewObject<UTestAlgorithm3>(this);
		Algorithm->Init(TerrainInformation);
		Algorithms.Add(Algorithm);
	}
}

void AHITerrainInstance::OnDataGenerated() 
{
	// TerrainData生成完地形数据，这边就可以开始Tick了。
	UE_LOG(LogHITerrain, Log, TEXT("AHITerrainInstance::OnDataGenerated"));
	PrimaryActorTick.SetTickFunctionEnable(true);
}

void AHITerrainInstance::Tick(float DeltaTime) 
{
	Super::Tick(DeltaTime);
	if(bFirstTick)
	{
		FRenderCommandFence Fence;
		Fence.BeginFence();
		Fence.Wait();
		while(!Data->bAvailable)
		{
			FPlatformProcess::Sleep(0.1);
		}
		Data->bAvailable = false;
		bFirstTick = false;
	}
	ChunkTicker->TickChunks();
}



