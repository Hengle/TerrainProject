#include "HITerrainData.h"
#include "HITerrainPerlinGenerator.h"


void UHITerrainData::InitData(const ETerrainType& TerrainType, const ENoiseType& TerrainNoiseType)
{
	if (TerrainNoiseType == ENoiseType::Perlin) 
	{
		TerrainNoiseGenerator = new HITerrainPerlinGenerator();
	}
	//TODO: 如果加了NoiseType加更多if
	else {
		
	}

	TerrainNoiseGenerator->Init(Seed);
	Size = FMath::Floor(ChunkSize / Step);
}

uint32 UHITerrainData::Run() 
{
	/*for (int32 X = 0; X < ChunkNum; X++) {
		for (int32 Y = 0; Y < ChunkNum; Y++) {
			GenerateChunk(X, Y);
		}
	}
	OnDataGenerated.ExecuteIfBound();*/
	OnDataGenerated.ExecuteIfBound();
	return 0;
}

const FChunkInformation& UHITerrainData::GetChunkData(const TPair<int32, int32>& Index)
{
	/*if (!bDataGenerated)
	{
		UE_LOG(LOGHITerrain, Error, TEXT("HITerrainData: Chunk Data[%d, %d] Not Generated!"), Index.Key, Index.Value)
	}*/
	if (ChunkData.Contains(Index)) 
	{

	}
	else 
	{
		GenerateChunk(Index.Key, Index.Value);
	}
	return ChunkData[Index];
}

void UHITerrainData::RemoveChunkData(const TPair<int32, int32>& Index)
{

}

void UHITerrainData::GenerateChunk(int32 X, int32 Y) 
{
	TPair<int32, int32> Index(X, Y);
	ChunkData.Add(Index, FChunkInformation());
	GenerateChunkVertices(Index);
	GenerateChunkTriangles(Index);
	GenerateChunkNormals(Index);
	GenerateChunkUVs(Index);
	GenerateChunkColors(Index);
	GenerateChunkTangents(Index);
	UE_LOG(LOGHITerrain, Log, TEXT("HITerrainData: Chunk[%d, %d] Generated!"), Index.Key, Index.Value)
	bDataGenerated = true;
}

void UHITerrainData::GenerateChunkVertices(const TPair<int32, int32>& Index)
{
	float RecentX = 0, RecentY = 0;
	TArray<FVector>* Vertices = &(ChunkData[Index].Vertices);
	for (int32 i = 0; i <= Size; i++) {
		for (int32 j = 0; j <= Size; j++) {
			float LocationX = Size * Step * Index.Key + RecentX;
			float LocationY = Size * Step * Index.Value + RecentY;
			float LocationZ = HeightScale * TerrainNoiseGenerator->GetValue((RecentX / Size / Step + Index.Key) * PositionScale,
															  (RecentY / Size / Step + Index.Value) * PositionScale);
			Vertices->Add(FVector(LocationX, LocationY, LocationZ));
			RecentY += Step;
		}
		RecentX += Step;
		RecentY = 0.0f;
	}
}

void UHITerrainData::GenerateChunkTriangles(const TPair<int32, int32>& Index)
{
	int32 Vertice1 = 0;
	int32 Vertice2 = 1;
	int32 Vertice3 = Size + 1;
	int32 Vertice4 = Size + 2;
	TArray<int32>* Triangles = &(ChunkData[Index].Triangles);
	for (int32 i = 0; i < Size; i++) {
		for (int32 j = 0; j < Size; j++) {
			Triangles->Add(Vertice1);
			Triangles->Add(Vertice2);
			Triangles->Add(Vertice4);
			Triangles->Add(Vertice4);
			Triangles->Add(Vertice3);
			Triangles->Add(Vertice1);
			Vertice1++;
			Vertice2++;
			Vertice3++;
			Vertice4++;
		}
		Vertice1++;
		Vertice2++;
		Vertice3++;
		Vertice4++;
	}
}

void UHITerrainData::GenerateChunkNormals(const TPair<int32, int32>& Index)
{
	TArray<FVector>* Normals = &(ChunkData[Index].Normals);
	for (int32 i = 0; i <= Size; i++) {
		for (int32 j = 0; j <= Size; j++) {
			Normals->Add(FVector(0, 0, 1));
		}
	}
}

void UHITerrainData::GenerateChunkUVs(const TPair<int32, int32>& Index)
{
	TArray<FVector2D>* UV0 = &(ChunkData[Index].UV0);
	float UVStepX = 1 / (Size + 1), UVStepY = 1 / (Size + 1);
	float RecentX = 0, RecentY = 0;
	for (int32 i = 0; i <= Size; i++) {
		for (int32 j = 0; j <= Size; j++) {
			RecentY += UVStepY;
			UV0->Add(FVector2D(RecentX, RecentY));
		}
		RecentX += UVStepX;
		RecentY = 0.0f;
	}
}

void UHITerrainData::GenerateChunkTangents(const TPair<int32, int32>& Index)
{
	TArray<FProcMeshTangent>* Tangents = &(ChunkData[Index].Tangents);
	for (int32 i = 0; i <= Size; i++) {
		for (int32 j = 0; j <= Size; j++) {
			Tangents->Add(FProcMeshTangent(1, 0, 0));
		}
	}
}

void UHITerrainData::GenerateChunkColors(const TPair<int32, int32>& Index)
{
	TArray<FLinearColor>* VertexColors = &(ChunkData[Index].VertexColors);
	for (int32 i = 0; i <= Size; i++) {
		for (int32 j = 0; j <= Size; j++) {
			VertexColors->Add(FLinearColor(0.5, 0.5, 0.5, 1.0));
		}
	}
}