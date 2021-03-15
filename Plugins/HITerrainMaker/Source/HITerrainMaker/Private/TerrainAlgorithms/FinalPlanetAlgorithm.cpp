// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainAlgorithms/FinalPlanetAlgorithm.h"



#include "TerrainAlgorithms/RiverPositionsAlgorithm.h"
#include "TerrainAlgorithms/ScaledBadlandsTerrainAlgorithm.h"
#include "TerrainAlgorithms/ScaledHillyTerrainAlgorithm.h"
#include "TerrainAlgorithms/ScaledMountainousTerrainAlgorithm.h"
#include "TerrainAlgorithms/ScaledPlainsTerrainAlgorithm.h"
#include "TerrainAlgorithms/TerrainTypeDefinitionAlgorithm.h"
#include "TerrainDatas/HITerrainData.h"

void UFinalPlanetAlgorithm::Init(FTerrainInformationPtr InInformation)
{
	UTerrainTypeDefinitionAlgorithm* TerrainTypeDefinitionAlgorithm = NewObject<UTerrainTypeDefinitionAlgorithm>(this);
	UScaledMountainousTerrainAlgorithm* ScaledMountainousTerrainAlgorithm = NewObject<UScaledMountainousTerrainAlgorithm>(this);
	UScaledHillyTerrainAlgorithm* ScaledHillyTerrainAlgorithm = NewObject<UScaledHillyTerrainAlgorithm>(this);
	UScaledPlainsTerrainAlgorithm* ScaledPlainsTerrainAlgorithm = NewObject<UScaledPlainsTerrainAlgorithm>(this);
	UScaledBadlandsTerrainAlgorithm* ScaledBadlandsTerrainAlgorithm = NewObject<UScaledBadlandsTerrainAlgorithm>(this);
	URiverPositionsAlgorithm* RiverPositionsAlgorithm = NewObject<URiverPositionsAlgorithm>(this);
	SubAlgorithms.Add(TerrainTypeDefinitionAlgorithm);
	SubAlgorithms.Add(ScaledMountainousTerrainAlgorithm);
	SubAlgorithms.Add(ScaledHillyTerrainAlgorithm);
	SubAlgorithms.Add(ScaledPlainsTerrainAlgorithm);
	SubAlgorithms.Add(ScaledBadlandsTerrainAlgorithm);
	SubAlgorithms.Add(RiverPositionsAlgorithm);
	
	Super::Init(InInformation);
	
    continentalShelf_te.SetSourceModule (0, TerrainTypeDefinitionAlgorithm->GetContinentDef());
    continentalShelf_te.AddControlPoint (-1.0);
    continentalShelf_te.AddControlPoint (-0.75);
    continentalShelf_te.AddControlPoint (Information->ShelfLevel);
    continentalShelf_te.AddControlPoint (1.0);

    
    continentalShelf_rm.SetSeed (Information->Seed + 130);
    continentalShelf_rm.SetFrequency (Information->ContinentFrequency * 4.375);
    continentalShelf_rm.SetLacunarity (Information->ContinentLacunarity);
    continentalShelf_rm.SetOctaveCount (16);
    continentalShelf_rm.SetNoiseQuality (noise::QUALITY_BEST);

    
    continentalShelf_sb.SetSourceModule (0, continentalShelf_rm);
    continentalShelf_sb.SetScale (-0.125);
    continentalShelf_sb.SetBias (-0.125);

    
    continentalShelf_cl.SetSourceModule (0, continentalShelf_te);
    continentalShelf_cl.SetBounds (-0.75, Information->SeaLevel);

    
    continentalShelf_ad.SetSourceModule (0, continentalShelf_sb);
    continentalShelf_ad.SetSourceModule (1, continentalShelf_cl);

    
    continentalShelf.SetSourceModule (0, continentalShelf_ad);

    
    baseContinentElev_sb.SetSourceModule (0, TerrainTypeDefinitionAlgorithm->GetContinentDef());
    baseContinentElev_sb.SetScale (Information->ContinentHeightScale);
    baseContinentElev_sb.SetBias (0.0);

    
    baseContinentElev_se.SetSourceModule (0, baseContinentElev_sb);
    baseContinentElev_se.SetSourceModule (1, continentalShelf);
    baseContinentElev_se.SetControlModule (TerrainTypeDefinitionAlgorithm->GetContinentDef());
    baseContinentElev_se.SetBounds (Information->ShelfLevel - 1000.0, Information->ShelfLevel);
    baseContinentElev_se.SetEdgeFalloff (0.03125);


    baseContinentElev.SetSourceModule (0, baseContinentElev_se);

    
    continentsWithPlains_ad.SetSourceModule (0, baseContinentElev);
    continentsWithPlains_ad.SetSourceModule (1, ScaledPlainsTerrainAlgorithm->GetScaledPlainsTerrain());

    
    continentsWithPlains.SetSourceModule (0, continentsWithPlains_ad);

    
    continentsWithHills_ad.SetSourceModule (0, baseContinentElev);
    continentsWithHills_ad.SetSourceModule (1, ScaledHillyTerrainAlgorithm->GetScaledHillyTerrain());

    
    continentsWithHills_se.SetSourceModule (0, continentsWithPlains);
    continentsWithHills_se.SetSourceModule (1, continentsWithHills_ad);
    continentsWithHills_se.SetControlModule (TerrainTypeDefinitionAlgorithm->GetTerrainTypeDef());
    continentsWithHills_se.SetBounds (1.0 - Information->HillsAmount, 1001.0 - Information->HillsAmount);
    continentsWithHills_se.SetEdgeFalloff (0.25);


    continentsWithHills.SetSourceModule (0, continentsWithHills_se);

  
    continentsWithMountains_ad0.SetSourceModule (0, baseContinentElev);
    continentsWithMountains_ad0.SetSourceModule (1, ScaledMountainousTerrainAlgorithm->GetScaledMountainousTerrain());

   
    continentsWithMountains_cu.SetSourceModule (0, TerrainTypeDefinitionAlgorithm->GetContinentDef());
    continentsWithMountains_cu.AddControlPoint (                  -1.0, -0.0625);
    continentsWithMountains_cu.AddControlPoint (                   0.0,  0.0000);
    continentsWithMountains_cu.AddControlPoint (1.0 - Information->MountainsAmount,  0.0625);
    continentsWithMountains_cu.AddControlPoint (                   1.0,  0.2500);

  
    continentsWithMountains_ad1.SetSourceModule (0, continentsWithMountains_ad0);
    continentsWithMountains_ad1.SetSourceModule (1, continentsWithMountains_cu);

   
    continentsWithMountains_se.SetSourceModule (0, continentsWithHills);
    continentsWithMountains_se.SetSourceModule (1, continentsWithMountains_ad1);
    continentsWithMountains_se.SetControlModule (TerrainTypeDefinitionAlgorithm->GetTerrainTypeDef());
    continentsWithMountains_se.SetBounds (1.0 - Information->MountainsAmount,
    1001.0 - Information->MountainsAmount);
    continentsWithMountains_se.SetEdgeFalloff (0.25);


    continentsWithMountains.SetSourceModule (0, continentsWithMountains_se);


    continentsWithBadlands_pe.SetSeed (Information->Seed + 140);
    continentsWithBadlands_pe.SetFrequency (16.5);
    continentsWithBadlands_pe.SetPersistence (0.5);
    continentsWithBadlands_pe.SetLacunarity (Information->ContinentLacunarity);
    continentsWithBadlands_pe.SetOctaveCount (2);
    continentsWithBadlands_pe.SetNoiseQuality (noise::QUALITY_STD);


    continentsWithBadlands_ad.SetSourceModule (0, baseContinentElev);
    continentsWithBadlands_ad.SetSourceModule (1, ScaledBadlandsTerrainAlgorithm->GetScaledBadlandsTerrain());


    continentsWithBadlands_se.SetSourceModule (0, continentsWithMountains);
    continentsWithBadlands_se.SetSourceModule (1, continentsWithBadlands_ad);
    continentsWithBadlands_se.SetControlModule (continentsWithBadlands_pe);
    continentsWithBadlands_se.SetBounds (1.0 - Information->BadlandsAmount,
    1001.0 - Information->BadlandsAmount);
    continentsWithBadlands_se.SetEdgeFalloff (0.25);

  
    continentsWithBadlands_ma.SetSourceModule (0, continentsWithMountains);
    continentsWithBadlands_ma.SetSourceModule (1, continentsWithBadlands_se);

 
    continentsWithBadlands.SetSourceModule (0, continentsWithBadlands_ma);


    continentsWithRivers_sb.SetSourceModule (0, RiverPositionsAlgorithm->GetRiverPositions());
    continentsWithRivers_sb.SetScale (Information->RiverDepth / 2.0);
    continentsWithRivers_sb.SetBias (-Information->RiverDepth / 2.0);


    continentsWithRivers_ad.SetSourceModule (0, continentsWithBadlands);
    continentsWithRivers_ad.SetSourceModule (1, continentsWithRivers_sb);


    continentsWithRivers_se.SetSourceModule (0, continentsWithBadlands);
    continentsWithRivers_se.SetSourceModule (1, continentsWithRivers_ad);
    continentsWithRivers_se.SetControlModule (continentsWithBadlands);
    continentsWithRivers_se.SetBounds (Information->SeaLevel,
    Information->ContinentHeightScale + Information->SeaLevel);
    continentsWithRivers_se.SetEdgeFalloff (Information->ContinentHeightScale - Information->SeaLevel);


    continentsWithRivers.SetSourceModule (0, continentsWithRivers_se);


    unscaledFinalPlanet.SetSourceModule (0, continentsWithRivers);


    finalPlanet_sb.SetSourceModule (0, unscaledFinalPlanet);
    finalPlanet_sb.SetScale ((Information->MaxElev - Information->MinElev) / 2.0);
    finalPlanet_sb.SetBias (Information->MinElev + ((Information->MaxElev - Information->MinElev) / 2.0));


    finalPlanet.SetSourceModule (0, finalPlanet_sb);
}

void UFinalPlanetAlgorithm::Apply(UHITerrainData* Data)
{
	Super::Apply(Data);
	int32 Size = Data->Size();
	for(int32 i = 0; i < Size; i++)
	{
		for(int32 j = 0; j < Size; j++)
		{
			float Value = finalPlanet.GetValue(i * 0.001f, j * 0.001f, 0);
			Data->SetSample(i, j, Value);
		}
	}
}
