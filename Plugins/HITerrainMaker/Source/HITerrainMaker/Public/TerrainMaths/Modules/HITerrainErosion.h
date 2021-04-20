// Fill out your copyright notice in the Description page of Project Settings.

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
	~FHITerrainErosion();

	void SetNumIteration(int32 InNumIteration);
	
	void SetHydroErosionScale(float InHydroErosionScale);
	void SetRainAmount(float InRainAmount);
	void SetEvaporationAmount(float InEvaporationAmount);
	void SetHydroErosionAngle(float InHydroErosionAngle);

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
	int32 NumIteration;
	
	float HydroErosionScale;
	float RainAmount;
	float EvaporationAmount;
	float HydroErosionAngle;

	float ThermalErosionScale;
	
private:
	TSharedPtr<FHITerrainDataChannel> HeightChannel;
	TSharedPtr<FHITerrainDataChannel> WaterChannel;
	TSharedPtr<FHITerrainDataChannel> SedimentChannel;
	TSharedPtr<FHITerrainDataChannel> FluxChannel;
	TSharedPtr<FHITerrainDataChannel> VelocityChannel;
};
