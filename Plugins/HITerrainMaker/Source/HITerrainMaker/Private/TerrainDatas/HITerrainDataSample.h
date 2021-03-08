#pragma once
#include "CoreMinimal.h"
#include "HITerrainCommons.h"
#include "HITerrainDataBase.h"

class UHITerrainDataSample : public UHITerrainDataBase
{
public:
	virtual void InitData() override;

	virtual uint32 Run() override;


protected:
	virtual void GenerateChunkData(int32 X, int32 Y) override;
};