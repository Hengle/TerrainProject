// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "HITerrainModule.h"
#include "ShaderParameterStruct.h"

class HITERRAINMAKER_API FHITerrainSlopeGPU: public FHITerrainModule
{
public:
	virtual void ApplyModule(UHITerrainData* Data) override;
};

class HITERRAINMAKER_API FSlopeShader: public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FSlopeShader);
	SHADER_USE_PARAMETER_STRUCT(FSlopeShader, FGlobalShader);

	public:
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	
	SHADER_PARAMETER(int, Size)
	SHADER_PARAMETER_UAV(RWStructuredBuffer<float3>, TerrainSlopeData)
	
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