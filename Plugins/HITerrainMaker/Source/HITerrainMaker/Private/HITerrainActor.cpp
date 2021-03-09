#include "HITerrainActor.h"
#include "Providers/RuntimeMeshProviderStatic.h"

AHITerrainActor::AHITerrainActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMesh"));
	RootComponent = ProceduralMesh;
}

void AHITerrainActor::Initialize(UHITerrainDataBase* Data, const TPair<int32, int32>& InIndex)
{
	FChunkInformationPtr ChunkData = Data->GetChunkData(Index);
	Index = InIndex;
	URuntimeMeshProviderStatic* StaticProvider = NewObject<URuntimeMeshProviderStatic>(this, TEXT("RuntimeMeshProvider-Static"));
	if (StaticProvider)
	{
		// The static provider should initialize before we use it
		GetRuntimeMeshComponent()->Initialize(StaticProvider);

		StaticProvider->SetupMaterialSlot(0, TEXT("TriMat"), Material);

		TArray<FVector> Positions;
		TArray<FColor> Colors;
		TArray<int32> Triangles;
		TArray<FVector> Normals;
		TArray<FVector2D> TexCoords;
		TArray<FRuntimeMeshTangent> Tangents;
		GeneratePositions(Positions);
		GenerateTriangles(Triangles);
		GenerateNormals(Normals);
		GenerateTexCoords(TexCoords);
		GenerateTangents(Tangents);
		GenerateColors(Colors);
		StaticProvider->CreateSectionFromComponents(0, 0, 0, Positions, Triangles, Normals, TexCoords, Colors, Tangents, ERuntimeMeshUpdateFrequency::Infrequent, true);
	}
	//FChunkInformationPtr ChunkData = Data->GetChunkData(Index);
	//Index = InIndex;
	//TArray<FVector> Positions;
	//TArray<FColor> Colors;
	//TArray<int32> Triangles;
	//TArray<FVector> Normals;
	//TArray<FVector2D> TexCoords;
	//TArray<FProcMeshTangent> Tangents;
	//GeneratePositions(Positions);
	//GenerateTriangles(Triangles);
	//GenerateNormals(Normals);
	//GenerateTexCoords(TexCoords);
	//GenerateTangents(Tangents);
	//GenerateColors(Colors);
	//ProceduralMesh->CreateMeshSection(0, Positions, Triangles, Normals, TexCoords, Colors, Tangents, true);
}


void AHITerrainActor::GeneratePositions(TArray<FVector>& Positions)
{
	float RecentX = 0, RecentY = 0;
	for (int32 i = 0; i <= Size; i++) {
		for (int32 j = 0; j <= Size; j++) {
			float LocationX = Size * Step * Index.Key + RecentX;
			float LocationY = Size * Step * Index.Value + RecentY;
			float LocationZ = 0.0f;
			Positions.Add(FVector(LocationX, LocationY, LocationZ));
			RecentY += Step;
		}
		RecentX += Step;
		RecentY = 0.0f;
	}
}

void AHITerrainActor::GenerateTriangles(TArray<int32>& Triangles)
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

void AHITerrainActor::GenerateNormals(TArray<FVector>& Normals)
{
	for (int32 i = 0; i <= Size; i++) {
		for (int32 j = 0; j <= Size; j++) {
			Normals.Add(FVector(0, 0, 1));
		}
	}
}

void AHITerrainActor::GenerateTangents(TArray<FRuntimeMeshTangent>& Tangents)
{
	for (int32 i = 0; i <= Size; i++) {
		for (int32 j = 0; j <= Size; j++) {
			Tangents.Add(FRuntimeMeshTangent(1, 0, 0));
		}
	}
}

void AHITerrainActor::GenerateTexCoords(TArray<FVector2D>& TexCoords)
{
	float UVStepX = 1 / (Size + 1), UVStepY = 1 / (Size + 1);
	float RecentX = 0, RecentY = 0;
	for (int32 i = 0; i <= Size; i++) {
		for (int32 j = 0; j <= Size; j++) {
			RecentY += UVStepY;
			TexCoords.Add(FVector2D(RecentX, RecentY));
		}
		RecentX += UVStepX;
		RecentY = 0.0f;
	}
}

void AHITerrainActor::GenerateColors(TArray<FColor>& Colors)
{
	for (int32 i = 0; i <= Size; i++) {
		for (int32 j = 0; j <= Size; j++) {
			Colors.Add(FColor(127, 127, 127, 255));
		}
	}
}
