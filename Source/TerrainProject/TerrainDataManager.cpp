// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainDataManager.h"

// TODO: 更复杂的地形生成
float TerrainDataManager::GetTerrainData(float x, float y) {
	return PerlinNoise.GetValue(x * LocationScale, y * LocationScale, 0) * HeightScale;
}


TerrainDataManager::TerrainDataManager()
{
}

TerrainDataManager::~TerrainDataManager()
{
}
