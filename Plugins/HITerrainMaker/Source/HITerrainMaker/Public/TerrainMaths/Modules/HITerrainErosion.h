#pragma once

#include "CoreMinimal.h"

#include "HITerrainModule.h"

/**
 * 
 */
class HITERRAINMAKER_API FHITerrainErosion: public FHITerrainModule
{
public:
	FHITerrainErosion();
	virtual ~FHITerrainErosion() override;

	void SetSeed(int32 InSeed);
	
	void SetNumIteration(int32 InNumIteration);
	void SetDeltaTime(float InDeltaTime);
	void SetEnableHydroErosion(bool InBool);
	void SetEnableThermalErosion(bool InBool);
	
	void SetHydroErosionScale(float InHydroErosionScale);
	void SetRainAmount(float InRainAmount);
	void SetEvaporationAmount(float InEvaporationAmount);
	void SetHydroErosionAngle(float InHydroErosionAngle);
	void SetErosionScale(float InErosionScale);
	void SetDepositionScale(float InDepositionScale);
	void SetSedimentCapacityScale(float InSedimentCapacityScale);
	void SetNumFlowIteration(int32 InNumFlowIteration);
	

	void SetThermalErosionScale(float InThermalErosionScale);


	virtual void ApplyModule(UHITerrainData* Data) override;

	void ApplyInitialization();
	void ApplyRainSimulation();
	void ApplyFlowSimulation();
	void ApplyErosionDepositionSimulation();
	void ApplySedimentSimulation();
	void ApplyThermalErosionSimulation();
	void ApplyEvaporationSimulation();

private:
	int32 Seed;
	int32 NumIteration;
	float DeltaTime;
	bool bEnableHydroErosion;
	bool bEnableThermalErosion;
	
	float HydroErosionScale;
	float RainAmount;
	float EvaporationAmount;
	float HydroErosionAngle;
	float ErosionScale;
	float DepositionScale;
	float SedimentCapacityScale;
	int32 NumFlowIteration;

	float ThermalErosionScale;
	
private:
	TSharedPtr<FHITerrainChannel> HeightChannel;
	TSharedPtr<FHITerrainChannel> WaterChannel;
	TSharedPtr<FHITerrainChannel> SedimentChannel;
	TSharedPtr<FHITerrainChannel> FluxChannel;
	TSharedPtr<FHITerrainChannel> VelocityChannel;
	TSharedPtr<FHITerrainChannel> TerrainFluxChannel;
};
