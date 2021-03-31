﻿// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainAlgorithms/SampleAlgorithms/ScaledPlainsTerrainAlgorithm.h"

#include "TerrainAlgorithms/SampleAlgorithms/PlainsTerrainAlgorithm.h"

void UScaledPlainsTerrainAlgorithm::Init(FTerrainInformationPtr InInformation)
{
	UPlainsTerrainAlgorithm* PlainsTerrainAlgorithm = NewObject<UPlainsTerrainAlgorithm>(this);
	SubAlgorithms.Add(PlainsTerrainAlgorithm);
	Super::Init(InInformation);
	
	scaledPlainsTerrain_sb.SetSourceModule (0, PlainsTerrainAlgorithm->GetPlainsTerrain());
	scaledPlainsTerrain_sb.SetScale (0.00390625);
	scaledPlainsTerrain_sb.SetBias (0.0078125);
	
	scaledPlainsTerrain.SetSourceModule (0, scaledPlainsTerrain_sb);
}

void UScaledPlainsTerrainAlgorithm::ApplyAlgorithm(UHITerrainData* Data)
{
	Super::ApplyAlgorithm(Data);
}

const noise::module::Cache& UScaledPlainsTerrainAlgorithm::GetScaledPlainsTerrain()
{
	return scaledPlainsTerrain;
}
