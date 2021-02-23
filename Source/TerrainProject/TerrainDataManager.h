// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "noiselib/noise.h"

/**
 * 
 */
class TERRAINPROJECT_API TerrainDataManager
{
public:
	static TerrainDataManager& GetInstance() {
		static TerrainDataManager INSTANCE;
		return INSTANCE;
	}

	float GetTerrainData(float x, float y);

	void SetSeed(int32 Seed) {
		PerlinNoise.SetSeed(Seed);
	}

	void SetLocationScale(float locationScale) {
		this->LocationScale = locationScale;
	}

	void SetHeightScale(float heightScale) {
		this->HeightScale = heightScale;
	}

	~TerrainDataManager();

private:
	TerrainDataManager();


private:
	float LocationScale;

	float HeightScale;

	noise::module::Perlin PerlinNoise;
	
};
