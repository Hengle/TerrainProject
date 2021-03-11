#pragma once
#include "CoreMinimal.h"
#include "HITerrainCommons.h"
#include "RuntimeMeshProvider.h"
#include "TerrainDatas/HITerrainData.h"
#include "HITerrainProviderBase.generated.h"

UCLASS()
class UHITerrainProviderBase : public URuntimeMeshProvider 
{
	GENERATED_BODY()

public:
	void SetData(UHITerrainData* InData);

	void SetSize(int32 InSize);
	int32 GetSize();

	void SetStep(float InStep);
	float GetStep();

public:
	virtual void Initialize() override;

protected:
	int32 Size;
	float Step;
	UHITerrainData* Data;
	UMaterialInterface* Material;
};