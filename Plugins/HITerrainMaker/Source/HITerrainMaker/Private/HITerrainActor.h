#pragma once
#include "CoreMinimal.h"
#include "HITerrainCommons.h"
#include "RuntimeMeshActor.h"
#include "TerrainDatas/HITerrainData.h"
#include "HITerrainActor.generated.h"

UCLASS()
class AHITerrainActor: public ARuntimeMeshActor 
{
	GENERATED_BODY()

public:
	void Initialize(UHITerrainData* Data, FTerrainInformationPtr InTerrainInformation, const TPair<int32, int32>& Index);

	void DeleteChunk();
	
	void GenerateChunk(ELODLevel LODLevel);

	bool IsGenerated();

	void GeneratePositions(TArray<FVector>& Positions, ELODLevel LODLevel);

	void GenerateTriangles(TArray<int32>& Triangles, ELODLevel LODLevel);

	void GenerateNormals(TArray<FVector>& Normals, ELODLevel LODLevel);

	void GenerateTangents(TArray<FRuntimeMeshTangent>& Tangents, ELODLevel LODLevel);

	void GenerateTexCoords(TArray<FVector2D>& TexCoords, ELODLevel LODLevel);

	void GenerateColors(TArray<FColor>& Colors, ELODLevel LODLevel);


public:
	UPROPERTY(EditAnywhere)
	UMaterialInterface* Material;

	int32 Size;
	float Step;
	TPair<int32, int32> Index;

private:
	bool bGenerated = false;
	
	URuntimeMeshProviderStatic* StaticProvider;
	
	FChunkDataPtr ChunkData;

	FTerrainInformationPtr TerrainInformation;

};