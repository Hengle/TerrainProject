#pragma once

#include "CoreMinimal.h"

#include "TerrainAlgorithms/HITerrainAlgorithm.h"
#include "TerrainMaths/noiselib/noise.h"
#include "PerlinAlgorithm.generated.h"


UCLASS()
class HITERRAINMAKER_API UPerlinAlgorithm : public UHITerrainAlgorithm
{
	GENERATED_BODY()

public:
	virtual void Init(FTerrainInformationPtr InInformation) override;
	virtual void ApplyAlgorithm(UHITerrainData* Data) override;
	virtual void DebugAlgorithm(UHITerrainData* Data) override;

private:
	noise::module::Perlin BaseLandscape;
	noise::module::Curve BaseLandscape_Curve;
	noise::module::Perlin BaseLandscape2;
	noise::module::ScaleBias BaseLandscape2_ScaleBias;
	noise::module::Min BaseLandscape_Min;
	noise::module::Clamp BaseLandscape_Clamp;
	noise::module::Cache BaseLandscape_Cache;

	noise::module::Terrace BaseLandscape_Terrace;
};
