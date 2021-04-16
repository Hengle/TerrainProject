/*
* This is an independent project of an individual developer. Dear PVS-Studio, please check it.
* PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
*/
#include "HITerrainActor.h"
#include "Providers/RuntimeMeshProviderStatic.h"
#include "TerrainDatas/HITerrainChunkData.h"

void AHITerrainActor::Initialize(UHITerrainData* Data, FTerrainInformationPtr InTerrainInformation, const TPair<int32, int32>& InIndex)
{
	Index = InIndex;
	ChunkData = Data->GetChunkData(Index);
	TerrainInformation = InTerrainInformation;
	StaticProvider = NewObject<URuntimeMeshProviderStatic>(this, TEXT("RuntimeMeshProvider-Static"));
	if (StaticProvider)
	{
		GetRuntimeMeshComponent()->Initialize(StaticProvider);
	}
	if(TerrainInformation->bEnableLOD)
	{
		Size = TerrainInformation->ChunkSize / TerrainInformation->LODHighQuality;
		Step = TerrainInformation->LODHighQuality;
	}
	else
	{
		Size = TerrainInformation->ChunkSize / TerrainInformation->LODLowQuality;
		Step = TerrainInformation->LODLowQuality;
	}
}

void AHITerrainActor::DeleteChunk()
{
	if(StaticProvider)
	{
		StaticProvider->ClearSection(0, 0);
		bGenerated = false;
	}
}

void AHITerrainActor::GenerateChunk(ELODLevel LODLevel)
{
	if (StaticProvider){
		StaticProvider->SetupMaterialSlot(0, TEXT("Material"), Material);
		TArray<FVector> Positions;
		TArray<FColor> Colors;
		TArray<int32> Triangles;
		TArray<FVector> Normals;
		TArray<FVector2D> TexCoords;
		TArray<FRuntimeMeshTangent> Tangents;

		GeneratePositions(Positions, LODLevel);
		GenerateTriangles(Triangles, LODLevel);
		GenerateNormals(Normals, LODLevel);
		GenerateTexCoords(TexCoords, LODLevel);
		GenerateTangents(Tangents, LODLevel);
		GenerateColors(Colors, LODLevel);
		if(bGenerated)
		{
			StaticProvider->UpdateSectionFromComponents(0, 0, Positions, Triangles, Normals, TexCoords, Colors, Tangents);
		}
		else
		{
			StaticProvider->CreateSectionFromComponents(0, 0, 0, Positions, Triangles, Normals, TexCoords, Colors, Tangents, ERuntimeMeshUpdateFrequency::Frequent, true);
			bGenerated = true;
		}
		
	}
}

bool AHITerrainActor::IsGenerated()
{
	return bGenerated;
}


void AHITerrainActor::GeneratePositions(TArray<FVector>& Positions, ELODLevel LODLevel)
{
	float RecentX = 0, RecentY = 0;
	for (int32 i = 0; i <= Size; i++) {
		for (int32 j = 0; j <= Size; j++) {
			float LocationX = Size * Step * Index.Key + RecentX;
			float LocationY = Size * Step * Index.Value + RecentY;
			// float LocationZ = ChunkData->GetSampleValue(i, j);
			float LocationZ = ChunkData->GetHeightValue(LocationX, LocationY);
			Positions.Add(FVector(LocationX, LocationY, LocationZ));
			RecentY += Step;
		}
		RecentX += Step;
		RecentY = 0.0f;
	}
}

void AHITerrainActor::GenerateTriangles(TArray<int32>& Triangles, ELODLevel LODLevel)
{
	int32 Vertice1 = 0;
	int32 Vertice2 = 1;
	int32 Vertice3 = Size + 1;
	int32 Vertice4 = Size + 2;
	for (int32 i = 0; i < Size; i++) {
		for (int32 j = 0; j < Size; j++) {
			Triangles.Add(Vertice1);
			Triangles.Add(Vertice2);
			Triangles.Add(Vertice4);
			Triangles.Add(Vertice4);
			Triangles.Add(Vertice3);
			Triangles.Add(Vertice1);
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

void AHITerrainActor::GenerateNormals(TArray<FVector>& Normals, ELODLevel LODLevel)
{
	for (int32 i = 0; i <= Size; i++) {
		for (int32 j = 0; j <= Size; j++) {
			Normals.Add(FVector(0, 0, 1));
		}
	}
}

void AHITerrainActor::GenerateTangents(TArray<FRuntimeMeshTangent>& Tangents, ELODLevel LODLevel)
{
	for (int32 i = 0; i <= Size; i++) {
		for (int32 j = 0; j <= Size; j++) {
			Tangents.Add(FRuntimeMeshTangent(1, 0, 0));
		}
	}
}

void AHITerrainActor::GenerateTexCoords(TArray<FVector2D>& TexCoords, ELODLevel LODLevel)
{
	// 1个chunk一个uv的实现
	float UVStep = 1.0 / Size;
	float RecentX = 0, RecentY = 0;
	for (int32 i = 0; i <= Size; i++)
	{
		for (int32 j = 0; j <= Size; j++)
		{
			TexCoords.Add(FVector2D(RecentX, RecentY));
			RecentY += UVStep;
		}
		RecentX += UVStep;
		RecentY = 0.0;
	}
	
	
	// 1个格子一个uv的实现
	// bool bEvenX = false, bEvenY = false;
	// float RecentX = 0, RecentY = 0;
	// for (int32 i = 0; i <= Size; i++) {
	// 	for (int32 j = 0; j <= Size; j++) {
	// 		if(bEvenX && bEvenY){
	// 			TexCoords.Add(FVector2D(1, 1));
	// 		}
	// 		if (bEvenX && !bEvenY) {
	// 			TexCoords.Add(FVector2D(1, 0));
	// 		}
	// 		if (!bEvenX && bEvenY) {
	// 			TexCoords.Add(FVector2D(0, 1));
	// 		}
	// 		if (!bEvenX && !bEvenY) {
	// 			TexCoords.Add(FVector2D(0, 0));
	// 		}
	// 		bEvenX = !bEvenX;
	// 	}
	// 	bEvenY = !bEvenY;
	// 	bEvenX = false;
	// }
}

void AHITerrainActor::GenerateColors(TArray<FColor>& Colors, ELODLevel LODLevel)
{
	for (int32 i = 0; i <= Size; i++) {
		for (int32 j = 0; j <= Size; j++) {
			Colors.Add(FColor(127, 127, 127, 255));
		}
	}
}
