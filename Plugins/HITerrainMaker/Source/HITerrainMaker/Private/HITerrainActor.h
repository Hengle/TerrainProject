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
	void Initialize(UHITerrainData* Data, const TPair<int32, int32>& Index);

	void GeneratePositions(TArray<FVector>& Positions);

	void GenerateTriangles(TArray<int32>& Triangles);

	void GenerateNormals(TArray<FVector>& Normals);

	void GenerateTangents(TArray<FRuntimeMeshTangent>& Tangents);

	void GenerateTexCoords(TArray<FVector2D>& TexCoords);

	void GenerateColors(TArray<FColor>& Colors);


public:
	UPROPERTY(EditAnywhere)
	UMaterialInterface* Material;

	int32 Size;
	float Step;
	TPair<int32, int32> Index;

private:
	FChunkDataPtr ChunkData;

};