#include "HITerrainChunkData.h"
#include "HITerrainData.h"

float FHITerrainChunkData::GetSampleValue(int32 X, int32 Y) 
{
	if (X < 0 || X > ChunkSize || Y < 0 || Y > ChunkSize)
	{
		UE_LOG(LogHITerrain, Error, TEXT("FHITerrainChunkData::GetSample Out Of Range! [%d, %d]"), X, Y)
		return 0.0f;
	}
	else 
	{
		return Data->GetSampleValue(Index.Key * ChunkSize + X, Index.Value * ChunkSize + Y);
	}
}

float FHITerrainChunkData::GetSampleValue(float X, float Y)
{
	// TODO: 和TerrainData的那个一起做
	return 0.0f;
}

ESampleType FHITerrainChunkData::GetSampleType(float X, float Y)
{
	// TODO: 和TerrainData的那个一起做
	return ESampleType::NONE;
}

ESampleType FHITerrainChunkData::GetSampleType(int32 X, int32 Y)
{
	if (X < 0 || X > ChunkSize || Y < 0 || Y > ChunkSize)
	{
		UE_LOG(LogHITerrain, Error, TEXT("FHITerrainChunkData::GetSample Out Of Range! [%d, %d]"), X, Y)
		return ESampleType::NONE;
	}
	else 
	{
		return Data->GetSampleType(Index.Key * ChunkSize + X, Index.Value * ChunkSize + Y);
	}
}
