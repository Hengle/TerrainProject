#pragma once

#include "CoreMinimal.h"

#include "TerrainAlgorithms/HITerrainAlgorithm.h"
#include "TerrainMaths/noiselib/noise.h"
#include "RidgedMultiAlgorithm.generated.h"

UCLASS()
class HITERRAINMAKER_API URidgedMultiAlgorithm : public UHITerrainAlgorithm
{
	GENERATED_BODY()

public:
	virtual void Init(FTerrainInformationPtr InInformation) override;
	virtual void ApplyAlgorithm(UHITerrainData* Data) override;
	virtual void DebugAlgorithm(UHITerrainData* Data) override;

private:
	noise::module::RidgedMulti RiverPositions_RidgedMulti;
	noise::module::Curve RiverPositions_Curve;
	noise::module::Cache RiverPositions;
};
