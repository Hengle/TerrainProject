#pragma once
#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "RuntimeMeshCore.h"
#include "HITerrainCommons.generated.h"

DECLARE_LOG_CATEGORY_CLASS(LOGHITerrain, Log, All)

UENUM()
enum class ETerrainType: uint8
{
	None = 0,
	Flat_Earth,			// 平面类地地形
	Globe_Earth,		// 球面类地地形 TODO
};

UENUM()
enum class ENoiseType : uint8
{
	None = 0,
	Perlin,				// Perlin噪声
};

UENUM()
enum class EBiomeType : uint8
{
	None = 0,
	Grass,
	Forest,
	Mountain,
	River,
	Lake,
	Ocean,
};

const float FLAT_CHUNK_INTERVAL = 0.1;
const float FLAT_RENDER_DISTANCE = 50001;
const int32 FLAT_RENDER_CHUNKNUM = 10;
const float FLAT_CHUNK_SIZE = 5000;
const float FLAT_VERTICE_SIZE_HIGH = 20;
const float FLAT_VERTICE_SIZE_MEDIUM = 50;
const float FLAT_VERTICE_SIZE_LOW = 100;

USTRUCT()
struct HITERRAINMAKER_API FTerrainInformation
{
	GENERATED_USTRUCT_BODY()

	FVector Position;
	ETerrainType TerrainType;
	ENoiseType TerrainNoiseType;
	int32 ChunkNum;
	int32 Seed;
	float Height;
	float Scale;
};

struct FChunkInformation
{
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UV0;
	//TArray<FProcMeshTangent> Tangents;
	TArray<FRuntimeMeshTangent> Tangents;
	TArray<FLinearColor> VertexColors;
};