#pragma once
#include "CoreMinimal.h"
#include "HITerrainAlgorithm.h"
#include "TerrainMaths/HITerrainPerlinGenerator.h"
#include "PlainAlgorithm.generated.h"

UCLASS()
class HITERRAINMAKER_API UPlainAlgorithm: public UHITerrainAlgorithm
{
	GENERATED_BODY()

public:
	void SetPlainData(int32 InSeed, float InPlainHeight, float InPlainScale, float InPlainThreshold);

	virtual void Apply(UHITerrainData* Data) override;

private:
	float PlainHeight;
	float PlainScale;
	float PlainThreshold;
	HITerrainPerlinGenerator PlainGenerator;
};
