// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HITerrainNoiseModule.h"

/**
 * 
 */
class FSampleModule: public HITerrainNoiseModule
{
public:
	FSampleModule();
	~FSampleModule();

public:
	virtual void Init() override;
	virtual float GetValue(int32 X, int32 Y) override;

public:
	void SetSeed(int32 InSeed)
	{
		this->Seed = InSeed;
	}

	void SetFrequency(float InFrequency)
	{
		this->Frequency = InFrequency;
	}

	void SetLacunarity(float InLacunarity)
	{
		this->Lacunarity = InLacunarity;
	}

	void SetSeaLevel(float InSeaLevel)
	{
		this->SeaLevel = InSeaLevel;
	}


private:
	int32 Seed;
	float Frequency;
	float Lacunarity;
	float SeaLevel;

	noise::module::Perlin baseContinentDef_pe0;
	noise::module::Curve baseContinentDef_cu;
	noise::module::Perlin baseContinentDef_pe1;
	noise::module::ScaleBias baseContinentDef_sb;
	noise::module::Min baseContinentDef_mi;
	noise::module::Clamp baseContinentDef_cl;
};
