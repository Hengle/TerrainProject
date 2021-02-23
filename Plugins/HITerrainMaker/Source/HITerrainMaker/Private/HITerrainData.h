#pragma once
#include "CoreMinimal.h"
#include "HITerrainCommons.h"
#include "HITerrainNoiseGenerator.h"
#include "noiselib/noise.h"
#include "HITerrainData.generated.h"

DECLARE_DELEGATE(OnDataGeneratedEvent)

UCLASS()
class UHITerrainData: public UObject, public FRunnable
{
	GENERATED_BODY()

public:
	void InitData(const ETerrainType& TerrainType, const ENoiseType& TerrainNoiseType);

	virtual uint32 Run() override;

	const FChunkInformation& GetChunkData(const TPair<int32, int32>& Index);

	void RemoveChunkData(const TPair<int32, int32>& Index);

private:
	void GenerateChunk(int32 X, int32 Y);

	void GenerateChunkVertices(const TPair<int32, int32>& Index);

	void GenerateChunkTriangles(const TPair<int32, int32>& Index);

	void GenerateChunkNormals(const TPair<int32, int32>& Index);

	void GenerateChunkUVs(const TPair<int32, int32>& Index);

	void GenerateChunkTangents(const TPair<int32, int32>& Index);

	void GenerateChunkColors(const TPair<int32, int32>& Index);

public:
	int32 Seed;
	int32 ChunkNum;
	float HeightScale;
	float PositionScale;
	OnDataGeneratedEvent OnDataGenerated;

private:
	bool bDataGenerated = false;
	float Step = FLAT_VERTICE_SIZE_MEDIUM;
	float ChunkSize = FLAT_CHUNK_SIZE;
	int32 Size;
	HITerrainNoiseGenerator* TerrainNoiseGenerator;
	TMap<TPair<int32, int32>, FChunkInformation> ChunkData;
};