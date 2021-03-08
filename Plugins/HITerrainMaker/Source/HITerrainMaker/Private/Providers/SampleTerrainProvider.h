#pragma once
#include "CoreMinimal.h"
#include "HITerrainCommons.h"
#include "HITerrainProviderBase.h"
#include "SampleTerrainProvider.generated.h"

UCLASS()
class USampleTerrainProvider : public UHITerrainProviderBase 
{
	GENERATED_BODY()

public:
	void SetIndex(const TPair<int32, int32>& InIndex);
	const TPair<int32, int32>& GetIndex();

public:
	bool GetSectionMeshForLOD(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData) override;

private:
	void GeneratePositions(FRuntimeMeshRenderableMeshData& MeshData);

	void GenerateTangents(FRuntimeMeshRenderableMeshData& MeshData);

	void GenerateTriangles(FRuntimeMeshRenderableMeshData& MeshData);

	void GenerateTexCoords(FRuntimeMeshRenderableMeshData& MeshData);

	void GenerateColors(FRuntimeMeshRenderableMeshData& MeshData);

private:
	TPair<int32, int32> Index;
};