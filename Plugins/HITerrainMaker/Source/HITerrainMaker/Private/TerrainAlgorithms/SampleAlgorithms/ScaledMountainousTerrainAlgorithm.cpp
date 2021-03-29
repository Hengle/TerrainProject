// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainAlgorithms/SampleAlgorithms/ScaledMountainousTerrainAlgorithm.h"

#include "TerrainAlgorithms/SampleAlgorithms/MountainousTerrainAlgorithm.h"
#include "TerrainDatas/HITerrainData.h"

void UScaledMountainousTerrainAlgorithm::Init(FTerrainInformationPtr InInformation)
{
	UMountainousTerrainAlgorithm* MountainousTerrainAlgorithm = NewObject<UMountainousTerrainAlgorithm>(this);
	SubAlgorithms.Add(MountainousTerrainAlgorithm);
	Super::Init(InInformation);
	
    scaledMountainousTerrain_sb0.SetSourceModule (0, MountainousTerrainAlgorithm->GetMountainousTerrain());
    scaledMountainousTerrain_sb0.SetScale (0.125);
    scaledMountainousTerrain_sb0.SetBias (0.125);

    
    scaledMountainousTerrain_pe.SetSeed (Information->Seed + 110);
    scaledMountainousTerrain_pe.SetFrequency (14.5);
    scaledMountainousTerrain_pe.SetPersistence (0.5);
    scaledMountainousTerrain_pe.SetLacunarity (Information->SG_MountainLacunarity);
    scaledMountainousTerrain_pe.SetOctaveCount (6);
    scaledMountainousTerrain_pe.SetNoiseQuality (noise::QUALITY_STD);

 
    scaledMountainousTerrain_ex.SetSourceModule (0, scaledMountainousTerrain_pe);
    scaledMountainousTerrain_ex.SetExponent (1.25);

 
    scaledMountainousTerrain_sb1.SetSourceModule (0,
    scaledMountainousTerrain_ex);
    scaledMountainousTerrain_sb1.SetScale (0.25);
    scaledMountainousTerrain_sb1.SetBias (1.0);


    scaledMountainousTerrain_mu.SetSourceModule (0,
    scaledMountainousTerrain_sb0);
    scaledMountainousTerrain_mu.SetSourceModule (1,
    scaledMountainousTerrain_sb1);


    scaledMountainousTerrain.SetSourceModule (0, scaledMountainousTerrain_mu);
}

void UScaledMountainousTerrainAlgorithm::Apply(UHITerrainData* Data)
{
	Super::Apply(Data);
	int32 Size = Data->Size();
	for(int32 i = 0; i < Size; i++)
	{
		for(int32 j = 0; j < Size; j++)
		{
			float Value = scaledMountainousTerrain.GetValue(i * 0.0001f, j * 0.0001f, 0) * 1000;
			Data->SetSampleValue(i, j, Value);
		}
	}
}

const noise::module::Cache& UScaledMountainousTerrainAlgorithm::GetScaledMountainousTerrain()
{
	return scaledMountainousTerrain;
}
