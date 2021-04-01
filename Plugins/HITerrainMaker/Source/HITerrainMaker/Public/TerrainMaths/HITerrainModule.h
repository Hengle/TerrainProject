#pragma once


#include "CoreMinimal.h"
#include "HITerrainCommons.h"

/*
 * 项目的数学模块类
 * 参考libnoise的模块实现
 */
class HITERRAINMAKER_API FHITerrainModule
{
public:
	void SetSourceModule(FHITerrainModule* SourceModule);
	
public:
	virtual float GetValue(float X, float Y);

	virtual int32 GetSourceModuleCapacity();

public:
	FHITerrainModule();
	virtual ~FHITerrainModule();

protected:
	TArray<FHITerrainModule*> SourceModules;
};