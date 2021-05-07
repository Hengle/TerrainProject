// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "HITerrainModule.h"
#include "ShaderParameterStruct.h"

class HITERRAINMAKER_API FErosionShader: public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FErosionShader);
	SHADER_USE_PARAMETER_STRUCT(FErosionShader, FGlobalShader);

public:
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	
	SHADER_PARAMETER(int, Size)
	SHADER_PARAMETER(int, NumIteration)
	SHADER_PARAMETER(int, CurrentIteration)
	SHADER_PARAMETER(float, DeltaTime)
	SHADER_PARAMETER(int, GBEnableHydroErosion)
	SHADER_PARAMETER(int, GBEnableThermalErosion)
	SHADER_PARAMETER(float, HydroErosionScale)
	SHADER_PARAMETER(float, RainAmount)
	SHADER_PARAMETER(float, EvaporationAmount)
	SHADER_PARAMETER(float, HydroErosionAngle)
	SHADER_PARAMETER(float, ErosionScale)
	SHADER_PARAMETER(float, DepositionScale)
	SHADER_PARAMETER(float, SedimentCapacityScale)
	SHADER_PARAMETER(float, ThermalErosionScale)
	SHADER_PARAMETER_UAV(RWStructuredBuffer<float4>, TerrainData)
	SHADER_PARAMETER_UAV(RWStructuredBuffer<float4>, Flux)
	SHADER_PARAMETER_UAV(RWStructuredBuffer<float4>, TerrainFlux)
	SHADER_PARAMETER_UAV(RWStructuredBuffer<float3>, Velocity)
	SHADER_PARAMETER_UAV(RWStructuredBuffer<float4>, TempTerrainData)
	SHADER_PARAMETER_UAV(RWStructuredBuffer<float4>, TempFlux)
	SHADER_PARAMETER_UAV(RWStructuredBuffer<float4>, TempTerrainFlux)
	
	END_SHADER_PARAMETER_STRUCT()

	

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static inline void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}

private:
};

class HITERRAINMAKER_API FHITerrainErosionGPU: public FHITerrainModule
{
public:
	FHITerrainErosionGPU();
	virtual ~FHITerrainErosionGPU() override;
	
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

	void ApplyErosionShader(UHITerrainData* Data);

private:
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

	float ThermalErosionScale;
};
