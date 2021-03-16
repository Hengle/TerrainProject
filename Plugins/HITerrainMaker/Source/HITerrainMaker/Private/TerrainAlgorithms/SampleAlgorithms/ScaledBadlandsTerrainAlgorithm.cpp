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

void UScaledBadlandsTerrainAlgorithm::Apply(UHITerrainData* Data)
{
	Super::Apply(Data);
}

const noise::module::Cache& UScaledBadlandsTerrainAlgorithm::GetScaledBadlandsTerrain()
{
	return scaledBadlandsTerrain;	
}
