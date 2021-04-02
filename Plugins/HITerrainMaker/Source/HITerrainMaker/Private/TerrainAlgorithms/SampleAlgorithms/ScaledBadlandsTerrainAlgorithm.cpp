/*
* This is an independent project of an individual developer. Dear PVS-Studio, please check it.
* PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
*/
// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainAlgorithms/SampleAlgorithms/ScaledBadlandsTerrainAlgorithm.h"

#include "TerrainAlgorithms/SampleAlgorithms/BadLandsTerrainAlgorithm.h"

void UScaledBadlandsTerrainAlgorithm::Init(FTerrainInformationPtr InInformation)
{
	UBadLandsTerrainAlgorithm* BadLandsTerrainAlgorithm = NewObject<UBadLandsTerrainAlgorithm>(this);
	SubAlgorithms.Add(BadLandsTerrainAlgorithm);
	Super::Init(InInformation);
	
	scaledBadlandsTerrain_sb.SetSourceModule (0, BadLandsTerrainAlgorithm->GetBadLandsTerrain());
	scaledBadlandsTerrain_sb.SetScale (0.0625);
	scaledBadlandsTerrain_sb.SetBias (0.0625);

	
	scaledBadlandsTerrain.SetSourceModule (0, scaledBadlandsTerrain_sb);
}

void UScaledBadlandsTerrainAlgorithm::ApplyAlgorithm(UHITerrainData* Data)
{
	Super::ApplyAlgorithm(Data);
}

const noise::module::Cache& UScaledBadlandsTerrainAlgorithm::GetScaledBadlandsTerrain()
{
	return scaledBadlandsTerrain;	
}
