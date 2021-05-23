/*
* This is an independent project of an individual developer. Dear PVS-Studio, please check it.
* PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
*/
#include "HITerrainChunkData.h"
#include "HITerrainData.h"
#include "Kismet/KismetMathLibrary.h"

float FHITerrainChunkData::GetHeightValue(int32 X, int32 Y) 
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

float FHITerrainChunkData::GetHeightValue(float X, float Y)
{
	return Data->GetHeightValue(X, Y);
}

float FHITerrainChunkData::GetChannelFloatValue(FString ChannelName, int32 X, int32 Y)
{
	auto Channel = Data->GetChannel(ChannelName);
	return Channel->GetFloat(Index.Key * ChunkSize + X, Index.Value * ChunkSize + Y);
}

float FHITerrainChunkData::GetChannelFloatValue(FString ChannelName, float X, float Y)
{
	float Value;
	Data->GetChannelValue(ChannelName, (int32)(X / 100.0f), (int32)(Y / 100.0f), Value);
	return Value;
}

FVector2D FHITerrainChunkData::GetUV(float X, float Y, float Tile)
{
	FVector2D UV;
	UV.X = (X - Index.Key * ChunkSize * 100.0f) / (ChunkSize * 100.0f);
	UV.Y = (Y - Index.Value * ChunkSize * 100.0f) / (ChunkSize * 100.0f);
	return UV * Tile;
}

float FHITerrainChunkData::GetChunkSize()
{
	return ChunkSize * 100.0f;
}

int32 FHITerrainChunkData::GetInnerPointSize(const ELODLevel& LODLevel)
{
	if(LODLevel == ELODLevel::LOD_LOW)
	{
		return ChunkSize / 4 - 3;
	}
	else if(LODLevel == ELODLevel::LOD_MEDIUM)
	{
		return ChunkSize / 2 - 3;
	}
	else if(LODLevel == ELODLevel::LOD_HIGH)
	{
		return ChunkSize - 3;
	}
	else // ELODLevel::None
	{
		return ChunkSize - 3;
	}
}

int32 FHITerrainChunkData::GetMediumPointSize(const ELODLevel& LODLevel)
{
	return GetInnerPointSize(LODLevel) + 2;
}

int32 FHITerrainChunkData::GetOuterPointSize(const ELODLevel& LODLevel)
{
	return GetMediumPointSize(ELODLevel::LOD_HIGH) + 2;
}

int32 FHITerrainChunkData::GetOuterPointScale(const ELODLevel& LODLevel)
{
	if(LODLevel == ELODLevel::LOD_LOW)
	{
		return 4;
	}
	else if(LODLevel == ELODLevel::LOD_MEDIUM)
	{
		return 2;
	}
	else if(LODLevel == ELODLevel::LOD_HIGH)
	{
		return 1;
	}
	else // ELODLevel::None
	{
		return 1;
	}
}

int32 FHITerrainChunkData::GetPointSize(const ELODLevel& LODLevel)
{
	if(LODLevel == ELODLevel::LOD_LOW)
	{
		return ChunkSize / 4 + 1;
	}
	else if(LODLevel == ELODLevel::LOD_MEDIUM)
	{
		return ChunkSize / 2 + 1;
	}
	else if(LODLevel == ELODLevel::LOD_HIGH)
	{
		return ChunkSize + 1;
	}
	else // ELODLevel::None
	{
	return ChunkSize + 1;
	}
}

float FHITerrainChunkData::GetStepOfLODLevel(const ELODLevel& LODLevel)
{
	if(LODLevel == ELODLevel::LOD_LOW)
	{
		return 400.0f;
	}
	else if(LODLevel == ELODLevel::LOD_MEDIUM)
	{
		return 200.0f;
	}
	else if(LODLevel == ELODLevel::LOD_HIGH)
	{
		return 100.0f;
	}
	else // ELODLevel::None
	{
		return 100.0f;
	}
}

TArray<FVector>& FHITerrainChunkData::GetChunkGrass()
{
	return Data->GetChunkGrass(Index);
}

FRotator FHITerrainChunkData::GetRotatorAtLocation(const FVector& Location)
{
	float Delta = 50.0f;
	FVector LeftPoint(Location.X - Delta, Location.Y - Delta, 0.0f);
	FVector RightPoint(Location.X + Delta, Location.Y + Delta, 0.0f);
	FVector TopPoint(Location.X + Delta, Location.Y - Delta, 0.0f);
	FVector BottomPoint(Location.X - Delta, Location.Y + Delta, 0.0f);
	LeftPoint.Z = Data->GetHeightValue(LeftPoint.X, LeftPoint.Y) + Data->GetSedimentValue(LeftPoint.X, LeftPoint.Y);
	RightPoint.Z = Data->GetHeightValue(RightPoint.X, RightPoint.Y) + Data->GetSedimentValue(RightPoint.X, RightPoint.Y);
	TopPoint.Z = Data->GetHeightValue(TopPoint.X, TopPoint.Y) + Data->GetSedimentValue(TopPoint.X, TopPoint.Y);
	BottomPoint.Z = Data->GetHeightValue(BottomPoint.X, BottomPoint.Y) + Data->GetSedimentValue(BottomPoint.X, BottomPoint.Y);
	FVector XVector = (LeftPoint - RightPoint);
	FVector YVector = (TopPoint - BottomPoint);
	FVector Normal = XVector * YVector;
	Normal.Normalize(10000.0f);
	FRotator Rotator = UKismetMathLibrary::MakeRotFromX(Normal);
	return Rotator;
}
