#pragma once

#include "CoreMinimal.h"

#include "HITerrainAlgorithmConfig.h"
#include "HITerrainCommons.h"
#include "UObject/Object.h"
#include "HITerrainAlgorithm.generated.h"

/*
 * 地形算法类
 */
UCLASS()
class HITERRAINMAKER_API UHITerrainAlgorithm : public UObject
{
	GENERATED_BODY()

public:
	virtual void Init(FTerrainInformationPtr InInformation);

	virtual void ApplyAlgorithm(class UHITerrainData* Data);

	virtual void DebugAlgorithm(class UHITerrainData* Data);

	UHITerrainAlgorithmConfig* GetConfig();

protected:
	FTerrainInformationPtr Information;
	
	bool bIsInited = false;

	UPROPERTY()
	UHITerrainAlgorithmConfig* Config;
};
