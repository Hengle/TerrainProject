/*
* This is an independent project of an individual developer. Dear PVS-Studio, please check it.
* PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
*/
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
		return Data->GetHeightValue(Index.Key * ChunkSize + X, Index.Value * ChunkSize + Y);
	}
}

float FHITerrainChunkData::GetSampleValue(float X, float Y)
{
	return Data->GetHeightValue(X, Y);
}
