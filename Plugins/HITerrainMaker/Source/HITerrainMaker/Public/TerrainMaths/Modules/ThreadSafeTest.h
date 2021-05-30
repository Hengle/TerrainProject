#pragma once

#include "CoreMinimal.h"

#include "HITerrainModule.h"

class HITERRAINMAKER_API FThreadSafeTest : public FHITerrainModule
{
public:
	virtual void ApplyModule(UHITerrainData* Data) override;
};
