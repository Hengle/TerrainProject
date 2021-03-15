// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "HITerrainAlgorithm.h"
#include "TerrainMaths/noiselib/noise.h"
#include "FinalPlanetAlgorithm.generated.h"

/**
 * 
 */
UCLASS()
class HITERRAINMAKER_API UFinalPlanetAlgorithm : public UHITerrainAlgorithm
{
	GENERATED_BODY()

public:
	virtual void Init(FTerrainInformationPtr InInformation) override;
	virtual void Apply(UHITerrainData* Data) override;

private:
	noise::module::Terrace continentalShelf_te;
	noise::module::RidgedMulti continentalShelf_rm;
	noise::module::ScaleBias continentalShelf_sb;
	noise::module::Clamp continentalShelf_cl;
	noise::module::Add continentalShelf_ad;
	noise::module::Cache continentalShelf;
	noise::module::ScaleBias baseContinentElev_sb;
	noise::module::Select baseContinentElev_se;
	noise::module::Cache baseContinentElev;
	noise::module::Add continentsWithPlains_ad;
	noise::module::Cache continentsWithPlains;
	noise::module::Add continentsWithHills_ad;
	noise::module::Select continentsWithHills_se;
	noise::module::Cache continentsWithHills;
	noise::module::Add continentsWithMountains_ad0;
	noise::module::Curve continentsWithMountains_cu;
	noise::module::Add continentsWithMountains_ad1;
	noise::module::Select continentsWithMountains_se;
	noise::module::Cache continentsWithMountains;
	noise::module::Perlin continentsWithBadlands_pe;
	noise::module::Add continentsWithBadlands_ad;
	noise::module::Select continentsWithBadlands_se;
	noise::module::Max continentsWithBadlands_ma;
	noise::module::Cache continentsWithBadlands;
	noise::module::ScaleBias continentsWithRivers_sb;
	noise::module::Add continentsWithRivers_ad;
	noise::module::Select continentsWithRivers_se;
	noise::module::Cache continentsWithRivers;
	noise::module::Cache unscaledFinalPlanet;
	noise::module::ScaleBias finalPlanet_sb;
	noise::module::Cache finalPlanet;
};
