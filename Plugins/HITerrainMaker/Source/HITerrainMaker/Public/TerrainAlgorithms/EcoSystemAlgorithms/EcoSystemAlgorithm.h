#pragma once

#include "CoreMinimal.h"

#include "TerrainAlgorithms/HITerrainAlgorithm.h"
#include "TerrainMaths/Modules/HITerrainHumidityGPU.h"
#include "TerrainMaths/Modules/HITerrainPerlin.h"
#include "TerrainMaths/Modules/HITerrainSlopeGPU.h"
#include "TerrainMaths/Modules/HITerrainVoronoi.h"
#include "UObject/Object.h"
#include "EcoSystemAlgorithm.generated.h"

UCLASS()
class HITERRAINMAKER_API UEcoSystemAlgorithm : public UHITerrainAlgorithm
{
	GENERATED_BODY()

public:
	virtual void Init(FTerrainInformationPtr InInformation) override;
	virtual void ApplyAlgorithm(UHITerrainData* Data) override;
	virtual void DebugAlgorithm(UHITerrainData* Data) override;

	void GenerateChunkGrassData(UHITerrainData* Data, TPair<int32, int32>& Index);

	void CalculateSlope(UHITerrainData* Data);
	void CalculateUnderWaterTerrain(UHITerrainData* Data);
	void CalculateHumidity(UHITerrainData* Data);

private:
	FHITerrainPerlin GrassPerlin;
	FHITerrainHumidityGPU HumidityGPU;
	FHITerrainSlopeGPU SlopeGPU;
};
