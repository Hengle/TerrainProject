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

	void GenerateChunk1(TArray<FVector>& Positions, TArray<FVector2D>& TexCoords, TArray<FColor>& Colors, ELODLevel LODLevel);

	void GenerateChunk2(TArray<FVector>& Normals, TArray<FRuntimeMeshTangent>& Tangents, TArray<FColor>& Colors, ELODLevel LODLevel);

	void GenerateChunk3(TArray<int32>& Triangles, ELODLevel LODLevel);

	void GeneratePositions(TArray<FVector>& Positions, ELODLevel LODLevel);

	void GenerateTriangles(TArray<int32>& Triangles, ELODLevel LODLevel);

	void GenerateNormals(TArray<FVector>& Normals, ELODLevel LODLevel);

	void GenerateTangents(TArray<FRuntimeMeshTangent>& Tangents, ELODLevel LODLevel);

	void GenerateTexCoords(TArray<FVector2D>& TexCoords, ELODLevel LODLevel);

	void GenerateColors(TArray<FColor>& Colors, ELODLevel LODLevel);

	void GenerateWater(ELODLevel LODLevel);

	void GenerateWaterPositions(TArray<FVector>& Positions, ELODLevel LODLevel);

	void GenerateWaterTriangles(TArray<int32>& Triangles, ELODLevel LODLevel);

	void GenerateWaterNormals(TArray<FVector>& Normals, ELODLevel LODLevel);

	void GenerateWaterTangents(TArray<FRuntimeMeshTangent>& Tangents, ELODLevel LODLevel);

	void GenerateWaterTexCoords(TArray<FVector2D>& TexCoords, ELODLevel LODLevel);

	void GenerateWaterColors(TArray<FColor>& Colors, ELODLevel LODLevel);

public:
	UPROPERTY(EditAnywhere)
	UMaterialInterface* Material;

	UPROPERTY(EditAnywhere)
	UMaterialInterface* WaterMaterial;

	int32 Size;
	// float Step;
	TPair<int32, int32> Index;

private:
	bool bGenerated = false;
	bool bWaterGenerated = false;
	
	URuntimeMeshProviderStatic* StaticProvider;
	
	FChunkDataPtr ChunkData;

	FTerrainInformationPtr TerrainInformation;

};