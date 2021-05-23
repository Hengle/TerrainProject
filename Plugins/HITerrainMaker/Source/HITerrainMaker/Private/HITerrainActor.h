#pragma once
#include "CoreMinimal.h"
#include "HITerrainCommons.h"
#include "RuntimeMeshActor.h"
#include "Engine/StaticMeshActor.h"
#include "TerrainDatas/HITerrainData.h"
#include "HITerrainActor.generated.h"

UCLASS()
class AHITerrainActor: public ARuntimeMeshActor 
{
	GENERATED_BODY()

public:
	void Initialize(UHITerrainData* Data, FTerrainInformationPtr InTerrainInformation, const TPair<int32, int32>& Index);

	void DeleteChunk();
	
	void GenerateChunk(ELODLevel InLODLevel);

	const ELODLevel& GetLODLevel();

	bool IsGenerated();

	void GenerateChunk1(TArray<FVector>& Positions, TArray<FVector2D>& TexCoords, TArray<FLinearColor>& Colors, ELODLevel InLODLevel);

	void GeneratePointData(TArray<FVector>& Positions, TArray<FVector2D>& TexCoords, TArray<FLinearColor>& Colors, float RecentX, float RecentY);

	void GenerateChunk2(TArray<FVector>& Normals, TArray<FRuntimeMeshTangent>& Tangents, TArray<FLinearColor>& Colors, ELODLevel InLODLevel);

	void GenerateChunk3(TArray<int32>& Triangles, ELODLevel InLODLevel);

	void GenerateWater(ELODLevel InLODLevel);

	void GenerateWater1(TArray<FVector>& Positions, TArray<FVector2D>& TexCoords, TArray<FLinearColor>& Colors, ELODLevel InLODLevel);

	void GeneratePointWaterData(TArray<FVector>& Positions, TArray<FVector2D>& TexCoords, TArray<FLinearColor>& Colors, float RecentX, float RecentY);

	void GenerateWater2(TArray<FVector>& Normals, TArray<FRuntimeMeshTangent>& Tangents, TArray<FLinearColor>& Colors, ELODLevel InLODLevel);

	void GenerateWater3(TArray<int32>& Triangles, ELODLevel InLODLevel);

	void GenerateWaterPositions(TArray<FVector>& Positions, ELODLevel InLODLevel);

	void GenerateWaterTriangles(TArray<int32>& Triangles, ELODLevel InLODLevel);

	void GenerateWaterNormals(TArray<FVector>& Normals, ELODLevel InLODLevel);

	void GenerateWaterTangents(TArray<FRuntimeMeshTangent>& Tangents, ELODLevel InLODLevel);

	void GenerateWaterTexCoords(TArray<FVector2D>& TexCoords, ELODLevel InLODLevel);

	void GenerateWaterColors(TArray<FLinearColor>& Colors, ELODLevel InLODLevel);

	void GenerateVegetation(ELODLevel InLODLevel);

public:
	UPROPERTY(EditAnywhere)
	UMaterialInterface* Material;

	UPROPERTY(EditAnywhere)
	UMaterialInterface* WaterMaterial;

	TPair<int32, int32> Index;

private:
	bool bGenerated = false;
	bool bWaterGenerated = false;

	ELODLevel LODLevel;
	
	URuntimeMeshProviderStatic* StaticProvider;
	
	FChunkDataPtr ChunkData;

	FTerrainInformationPtr TerrainInformation;

	UPROPERTY()
	UStaticMesh* GrassStaticMesh = LoadObject<UStaticMesh>(this,TEXT("/Game/Light_Foliage/Meshes/SM_Grass_02.SM_Grass_02"));

	UPROPERTY()
	TArray<AStaticMeshActor*> StaticMeshActors;
};