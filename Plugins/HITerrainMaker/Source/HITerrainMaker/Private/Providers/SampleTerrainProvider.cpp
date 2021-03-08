#include "SampleTerrainProvider.h"

bool USampleTerrainProvider::GetSectionMeshForLOD(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData) 
{
	GeneratePositions(MeshData);
	GenerateTriangles(MeshData);
	GenerateTexCoords(MeshData);
	//GenerateTangents(MeshData);
	GenerateColors(MeshData);
	return false;
}

void USampleTerrainProvider::GeneratePositions(FRuntimeMeshRenderableMeshData& MeshData)
{
	float RecentX = 0, RecentY = 0;
	for (int32 i = 0; i <= Size; i++) {
		for (int32 j = 0; j <= Size; j++) {
			float LocationX = Size * Step * Index.Key + RecentX;
			float LocationY = Size * Step * Index.Value + RecentY;
			/*float LocationZ = Height * TerrainNoiseGenerator->GetValue((RecentX / Size / Step + Index.Key) * Scale,
				(RecentY / Size / Step + Index.Value) * Scale);*/
			float LocationZ = 0;
			MeshData.Positions.Add(FVector(LocationX, LocationY, LocationZ));
			RecentY += Step;
		}
		RecentX += Step;
		RecentY = 0.0f;
	}
}

void USampleTerrainProvider::GenerateTriangles(FRuntimeMeshRenderableMeshData& MeshData)
{
	int32 Vertice1 = 0;
	int32 Vertice2 = 1;
	int32 Vertice3 = Size + 1;
	int32 Vertice4 = Size + 2;
	for (int32 i = 0; i < Size; i++) {
		for (int32 j = 0; j < Size; j++) {
			MeshData.Triangles.AddTriangle(Vertice1, Vertice2, Vertice4);
			MeshData.Triangles.AddTriangle(Vertice4, Vertice3, Vertice1);
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

void USampleTerrainProvider::GenerateTangents(FRuntimeMeshRenderableMeshData& MeshData)
{
	for (int32 i = 0; i <= Size; i++) {
		for (int32 j = 0; j <= Size; j++) {
			//Tangents->Add(FProcMeshTangent(1, 0, 0));
			MeshData.Tangents.Add(FVector(0, 0, 1), FVector(1, 0, 0));
		}
	}
}

void USampleTerrainProvider::GenerateTexCoords(FRuntimeMeshRenderableMeshData& MeshData)
{
	float UVStepX = 1 / (Size + 1), UVStepY = 1 / (Size + 1);
	float RecentX = 0, RecentY = 0;
	for (int32 i = 0; i <= Size; i++) {
		for (int32 j = 0; j <= Size; j++) {
			RecentY += UVStepY;
			//MeshData.TexCoords(RecentX, RecentY);
		}
		RecentX += UVStepX;
		RecentY = 0.0f;
	}
}

void USampleTerrainProvider::GenerateColors(FRuntimeMeshRenderableMeshData& MeshData)
{
	for (int32 i = 0; i <= Size; i++) {
		for (int32 j = 0; j <= Size; j++) {
			MeshData.Colors.Add(FColor(127, 127, 127, 255));
		}
	}
}


void USampleTerrainProvider::SetIndex(const TPair<int32, int32>& InIndex)
{
	Index = InIndex;
}

const TPair<int32, int32>& USampleTerrainProvider::GetIndex()
{
	return Index;
}