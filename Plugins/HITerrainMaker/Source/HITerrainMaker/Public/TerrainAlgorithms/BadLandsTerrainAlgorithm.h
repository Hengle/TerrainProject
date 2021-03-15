// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "HITerrainAlgorithm.h"
#include "TerrainMaths/noiselib/noise.h"
#include "BadLandsTerrainAlgorithm.generated.h"

/**
 * 
 */
UCLASS()
class HITERRAINMAKER_API UBadLandsTerrainAlgorithm : public UHITerrainAlgorithm
{
	GENERATED_BODY()

public:
	virtual void Init(FTerrainInformationPtr InInformation) override;
	virtual void Apply(UHITerrainData* Data) override;

public:
	const noise::module::Cache& GetBadLandsTerrain();

private:
	noise::module::RidgedMulti badlandsSand_rm;
	noise::module::ScaleBias badlandsSand_sb0;
	noise::module::Voronoi badlandsSand_vo;
	noise::module::ScaleBias badlandsSand_sb1;
	noise::module::Add badlandsSand_ad;
	noise::module::Cache badlandsSand;
	noise::module::Perlin badlandsCliffs_pe;
	noise::module::Curve badlandsCliffs_cu;
	noise::module::Clamp badlandsCliffs_cl;
	noise::module::Terrace badlandsCliffs_te;
	noise::module::Turbulence badlandsCliffs_tu0;
	noise::module::Turbulence badlandsCliffs_tu1;
	noise::module::Cache badlandsCliffs;
	noise::module::ScaleBias badlandsTerrain_sb;
	noise::module::Max badlandsTerrain_ma;
	noise::module::Cache badlandsTerrain;
};
