#pragma once
#include "CoreMinimal.h"
#include "HITerrainCommons.h"
#include "HITerrainDataBase.h"
#include "HITerrainPerlinGenerator.h"
#include "HITerrainDataSample.generated.h"

UCLASS()
class UHITerrainDataSample : public UHITerrainDataBase
{
	GENERATED_BODY()
public:
	virtual void InitData() override;

	virtual uint32 Run() override;

public:
	


protected:
	virtual void GenerateChunkData(int32 X, int32 Y) override;

private:
	
};