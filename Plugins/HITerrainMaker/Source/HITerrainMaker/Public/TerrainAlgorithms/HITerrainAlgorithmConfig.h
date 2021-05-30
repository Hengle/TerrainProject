#pragma once

#include "CoreMinimal.h"
#include "HITerrainCommons.h"
#include "HITerrainAlgorithmConfig.generated.h"

/**
 * AlgorithmConfig，主要是存和Algorithm相关的一些配置信息，比如材质等（后面根据需求慢慢加）
 * 
 */
UCLASS()
class HITERRAINMAKER_API UHITerrainAlgorithmConfig: public UObject
{
	GENERATED_BODY()
	
public:
	virtual void ApplyConfig();

	virtual void DebugConfig();

protected:
	UMaterial* Material;
	UMaterialInstanceDynamic* MaterialInstance;
};
