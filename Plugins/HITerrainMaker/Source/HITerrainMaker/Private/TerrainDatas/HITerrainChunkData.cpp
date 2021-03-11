#include "HITerrainChunkData.h"
#include "HITerrainData.h"

float FHITerrainChunkData::GetSample(int32 X, int32 Y) 
{
	if (X < 0 || X > ChunkSize || Y < 0 || Y > ChunkSize)
	{
		UE_LOG(LogHITerrain, Error, TEXT("FHITerrainChunkData::GetSample Out Of Range! [%d, %d]"), X, Y)
		return 0.0f;
	}
	else 
	{
		return Data->GetSample(Index.Key * ChunkSize + X, Index.Value * ChunkSize + Y);
	}
}