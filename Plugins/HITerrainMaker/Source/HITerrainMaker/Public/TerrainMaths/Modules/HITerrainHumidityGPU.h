// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "HITerrainModule.h"
#include "ShaderParameterStruct.h"


class HITERRAINMAKER_API FHITerrainHumidityGPU : public FHITerrainModule
{
public:
	FHITerrainHumidityGPU();
	
	void SetNumIteration(int32 InNumIteration);
	
	virtual void ApplyModule(UHITerrainData* Data) override;

private:
	int32 NumIteration;
};

class HITERRAINMAKER_API FHumidityShader: public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FHumidityShader);
	SHADER_USE_PARAMETER_STRUCT(FHumidityShader, FGlobalShader);

	public:
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	
	SHADER_PARAMETER(int, Size)
	SHADER_PARAMETER(float, Step)
	SHADER_PARAMETER_UAV(RWStructuredBuffer<float2>, WaterHumidityData)
	
	END_SHADER_PARAMETER_STRUCT()
	
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static inline void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}
};