#pragma once
#include "CoreMinimal.h"
#include "HITerrainCommons.h"
#include "RuntimeMeshProvider.h"
#include "HITerrainProviderBase.generated.h"

UCLASS()
class UHITerrainProviderBase : public URuntimeMeshProvider 
{
	GENERATED_BODY()

public:
	void SetSize(int32 InSize);
	int32 GetSize();

	void SetStep(float InStep);
	float GetStep();

	void SetHeight(float InHeight);
	float GetHeight();

	void SetScale(float InScale);
	float GetScale();

public:
	void Initialize() override;

protected:
	int32 Size;
	float Step;
	float Height;
	float Scale;
	UMaterialInterface* Material;
};