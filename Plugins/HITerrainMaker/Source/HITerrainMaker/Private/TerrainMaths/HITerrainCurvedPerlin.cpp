/*
* This is an independent project of an individual developer. Dear PVS-Studio, please check it.
* PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
*/
// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainMaths/HITerrainCurvedPerlin.h"

void FHITerrainCurvedPerlin::Init(int32 InSeed/* = 10086*/, float InFrequency/* = 1.0f*/, float InLacunarity/* = 2.0f*/,
	int32 InOctaveCount/* = 6*/, float InPersistence/* = 0.5f*/)
{
	Perlin.SetSeed(InSeed);
	Perlin.SetFrequency(InFrequency);
	Perlin.SetLacunarity(InLacunarity);
	Perlin.SetOctaveCount(InOctaveCount);
	Perlin.SetPersistence(InPersistence);
	Curve.SetSourceModule(0, Perlin);
}

void FHITerrainCurvedPerlin::AddControlPoint(float InputValue, float OutputValue)
{
	Curve.AddControlPoint(InputValue, OutputValue);
	bUseControlPoint = true;
}

float FHITerrainCurvedPerlin::GetValue(float X, float Y)
{
	if(bUseControlPoint)
	{
		return Curve.GetValue(X, Y, 0.0f);
	}
	else
	{
		return Perlin.GetValue(X, Y, 0.0f);
	}
}


